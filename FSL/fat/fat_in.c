#include "fs_fat.h"
#include "fs_clib.h"
#include "fs_api.h"
#include "lbl.h"
#include "sdi.h"

/*********************************************************************
*
*             Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*             FS__fat_fread
*
  Description:
  FS internal function. Read data from a file.

  Parameters:
  pData       - Pointer to a data buffer for storing data transferred
                from file. 
  Size        - Size of an element to be transferred from file to data
                buffer
  N           - Number of elements to be transferred from the file.
  pFile       - Pointer to a FS_FILE data structure.
  
  Return value:
  Number of elements read.
*/
//3层循环，外1层找到需要的簇，外2层找到需要的扇区，最里层精确到字节位置

U32 FS__fat_fread(void *pData, U32 Size, U32 N, FS_FILE *pFile) {
  U32 todo;
  U32 i;
  U32 j;
  U32 fatsize;
  U32 fileclustnum;
  U32 diskclustnum;
  U32 prevclust;
  U32 dstart;
  U32 dsize;
  U32 datastart;
  S8 *buffer;
  S32 err;
                                            
  if (!pFile) {
      return 0;  /* No valid pointer to a FS_FILE structure */
  }
  buffer = (S8 *)FS__fat_malloc(FAT_SEC_SIZE);
  if ((S32)buffer==BUFFER_ERR) {
    return 0;
  }
  fatsize   = FS__pDiskInfo[0].FATSecCnt;
  dstart    = FS__pDiskInfo[0].RootDirTable;
  dsize     = FS__pDiskInfo[0].RootSecCnt;
  datastart = FS__pDiskInfo[0].DataStartSec;
  prevclust = 0;
  todo = N * Size;
  while (todo) {
    if (pFile->Offset >= pFile->FileSize) {
      /* EOF has been reached */
      FS__fat_free(buffer);
      return ((N * Size - todo) / Size);
    }
    fileclustnum = pFile->Offset / (FAT_SEC_SIZE * FS__pDiskInfo[0].SecPerClus);//确定要读的簇
    if (prevclust == 0) {
      diskclustnum = pFile->CurClust; 
      if (diskclustnum == 0) {
        /* Find current cluster by starting at 1st cluster of the file */
        diskclustnum = FS__fat_diskclust( pFile->FstClus, fileclustnum);
      }
    }
    else {
      /* Get next cluster of the file */
      diskclustnum = FS__fat_diskclust( prevclust, 1);//寻找要读取的簇的簇号
    }
    prevclust       = diskclustnum;
    pFile->CurClust = diskclustnum;
    if (diskclustnum == 0) {
      /* Could not find current cluster */
      FS__fat_free(buffer);
      return ((N * Size - todo) / Size);
    }
    diskclustnum -= 2;
    j = (pFile->Offset % (FAT_SEC_SIZE * FS__pDiskInfo[0].SecPerClus))/ FAT_SEC_SIZE;//确定找到的簇中要读的扇区
    while (1) {
      if (!todo) {
        break;  /* Nothing more to write */
      }
      if (j >= (U32)FS__pDiskInfo[0].SecPerClus) {
        break;  /* End of the cluster reached */
      }
      if (pFile->Offset >= pFile->FileSize) {
        break;  /* End of the file reached */
      }
      err = FS__lb_read(FS__pDevInfo[0].harddisk_driver, 0, datastart +
                                    diskclustnum *FS__pDiskInfo[0].SecPerClus + j, (void*)buffer);
      if (err !=SD_NO_ERR) {
        FS__fat_free(buffer);
        return ((N * Size - todo) / Size);
      }
      i = pFile->Offset % FAT_SEC_SIZE;//确定精确的读位置
      while (1) {
        if (!todo) {
          break;  /* Nothing more to write */
        }
        if (i >= FAT_SEC_SIZE) {
          break;  /* End of the sector reached */
        }
        if (pFile->Offset >= pFile->FileSize) {
          break;  /* End of the file reached */
        }
        *((char*)(((char*)pData) + N * Size - todo)) = buffer[i];
        i++;
        pFile->Offset++;
        todo--;
      }
      j++;
    }  /* Sector loop */
  }  /* Cluster loop */
  if (i >= FAT_SEC_SIZE) {
    if (j >= FS__pDiskInfo[0].SecPerClus) {
      pFile->CurClust = FS__fat_diskclust(prevclust, 1);
    }
  }
  FS__fat_free(buffer);
  return ((N * Size - todo) / Size);
}

