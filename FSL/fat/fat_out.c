#include "lbl.h"
#include "fs_int.h"
#include "fs_api.h"
#include "fs_fat.h"
#include "fs_clib.h"
#include "sdi.h"


/*********************************************************************
*
*             Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*             _FS_fat_write_dentry
*
  Description:
  FS internal function. Write a directory entry.

  Parameters:
  FirstClust  - First cluster of the file, which's directory entry 
                will be written.
  pDirEntry   - Pointer to an FS__fat_dentry_type structure, which 
                contains the new directory entry.
  DirSec      - Sector, which contains the directory entry.
  pBuffer     - Pointer to a buffer, which contains the sector with 
                the old directory entry.
 
  Return value:
  ==1         - Directory entry has been written.
  ==0         - An error has occured.
*/

static S32 _FS_fat_write_dentry( U32 FirstClust, FS__fat_dentry_type *pDirEntry, 
                                U32 DirSec, S8 *pBuffer) {
  FS__fat_dentry_type *s;
  U32 value;
  S32 err;

  if (DirSec == 0) {
    return 0;  /* Not a valid directory sector */
  }
  if (pBuffer == 0) {
    return 0;  /* No buffer */
  }
  /* Scan for the directory entry with FirstClust in the directory sector */
  s = (FS__fat_dentry_type*)pBuffer;
  while (1) {
    if (s >= (FS__fat_dentry_type*)(pBuffer + FAT_SEC_SIZE)) {
      break;  /* End of sector reached */
    }
    value = (U32)s->data[26] + 0x100UL * s->data[27] + 0x10000UL * s->data[20] + 0x1000000UL * s->data[21];
    if (value == FirstClust) {
      break;  /* Entry found */
    }
    s++;
  }
  if (s < (FS__fat_dentry_type*)(pBuffer + FAT_SEC_SIZE)) {
    if (pDirEntry) {
      FS__CLIB_memcpy(s, pDirEntry, sizeof(FS__fat_dentry_type));
      err = FS__lb_write(FS__pDevInfo[0].harddisk_driver, 0, DirSec, (void*)pBuffer);
      if (err !=SD_NO_ERR) {
        return 0;
      }
    }
    return 1;
  }
  return 0;
}


/*********************************************************************
*
*             _FS_fat_read_dentry
*
  Description:
  FS internal function. Read a directory entry.

  Parameters:
  FirstClust  - First cluster of the file, which's directory entry 
                will be read.
  DirStart    - Start of directory, where to read the entry.
  pDirEntry   - Pointer to an FS__fat_dentry_type structure, which is 
                used to read the directory entry.
  pDirSec     - Pointer to an FS_u32, which is used to store the sector
                number, in which the directory entry has been read.
  pBuffer     - Pointer to a buffer, which is used for reading the
                directory.
 
  Return value:
  ==1         - Directory entry has been read.
  ==0         - An error has occured.
*/

static S32 _FS_fat_read_dentry( U32 FirstClust, U32 DirStart, 
                                   FS__fat_dentry_type *pDirEntry, U32 *pDirSec, S8 *pBuffer) {
  U32 i;
  U32 dsize;
  U32 value;
  FS__fat_dentry_type *s;
  S32 err;

  if (pBuffer == 0) {
    return 0;
  }
  dsize  =  FS__fat_dir_size( DirStart);
  /* Read the directory */
  for (i = 0; i < dsize; i++) {
    *pDirSec = FS__fat_dir_realsec(DirStart, i);
    if (*pDirSec == 0) {
      return 0;  /* Unable to translate relative directory sector to absolute setor */
    }
    err = FS__lb_read(FS__pDevInfo[0].harddisk_driver,0, *pDirSec, (void*)pBuffer);
    if (err !=SD_NO_ERR) {
      return 0;
    }
    /* Scan for entry with FirstClus in the sector */
    s = (FS__fat_dentry_type*)pBuffer;
    while (1) {
      if (s >= (FS__fat_dentry_type*)(pBuffer + FAT_SEC_SIZE)) {
        break;  /* End of sector reached */
      }
      value = (U32)s->data[26] + 0x100UL * s->data[27] + 0x10000UL * s->data[20] + 0x1000000UL * s->data[21];
      if (value == FirstClust) {
        break;  /* Entry found */
      }
      s++;
    }
    if (s < (FS__fat_dentry_type*)(pBuffer + FAT_SEC_SIZE)) {
      if (pDirEntry) {
        /* Read the complete directory entry from the buffer */
        FS__CLIB_memcpy(pDirEntry, s, sizeof(FS__fat_dentry_type));
      }
      return 1;
    }
  }
  return 0;
}


/*********************************************************************
*
*             Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*             FS__fat_fwrite
*
  Description:
  FS internal function. Write data to a file.

  Parameters:
  pData       - Pointer to data, which will be written to the file. 
  Size        - Size of an element to be transferred to a file.
  N           - Number of elements to be transferred to the file.
  pFile       - Pointer to a FS_FILE data structure.
  
  Return value:
  Number of elements written.
*/

S32 FS__fat_fwrite(const void *pData, U32 Size, U32 N, FS_FILE *pFile) {
  U32 todo;
  U32 bytesperclus;
  U32 datastart;
  U32 fatsize;
  U32 fileclustnum;
  U32 diskclustnum;
  U32 prevclust;
  S32 last;
  S32 i;
  S32 j;
  FS__fat_dentry_type s;
  U32 dsec;
  S32 err;
  S32 lexp;
  S8 *buffer;

  if (!pFile) {
      return 0;
  }
  /* Check if media is OK */
  buffer = (S8 *)FS__fat_malloc(FAT_SEC_SIZE);
  if ((S32)buffer==BUFFER_ERR) {
    return 0;
  }
  fatsize = FS__pDiskInfo[0].FATSecCnt;
  todo = N * Size;  /* Number of bytes to be written */
  if (!todo) {
    FS__fat_free(buffer);
    return 0;
  }
  /* Alloc new clusters if required */
  bytesperclus = (FS__pDiskInfo[0].SecPerClus *FS__pDiskInfo[0].BytsPerSec);
  /* Calculate number of clusters required */
  i = (pFile->Offset + todo) / bytesperclus;
  if ((pFile->Offset + todo) % bytesperclus) {
    i++;
  }
  /* Calculate clusters already allocated */
  j = pFile->FileSize / bytesperclus;
  lexp = (pFile->FileSize % bytesperclus);
  lexp = lexp || (pFile->FileSize == 0);
  if (lexp) {
    j++;
  }
  i -= j;
  if (i > 0) {
    /* Alloc new clusters */
    last = pFile->EOFClust;
    if (last < 0) {
      /* Position of EOF is unknown, so we scan the whole file to find it */
      last = FS__fat_FAT_find_eof(pFile->FstClus, 0);
    }
    if (last < 0) {
      /* No EOF found */
      FS__fat_free(buffer);
      return 0;
    }
    while (i) {
      last = FS__fat_FAT_alloc(last);  /* Allocate new cluster */
      pFile->EOFClust = last;
      if (last < 0) {
        /* Cluster allocation failed */
        pFile->FileSize += (N * Size - todo);
        FS__fat_free(buffer);
        return ((N * Size - todo) / Size);
      }
      i--;
    }
  }
  /* Get absolute postion of data area on the media */
  datastart = FS__pDiskInfo[0].DataStartSec;
  /* Write data to clusters */
  prevclust = 0;
  while (todo) {  /* Write data loop */
    /* Translate file ppinter position to cluster position*/
    fileclustnum = pFile->Offset / bytesperclus;
    /* 
       Translate the file relative cluster position to an absolute cluster
       position on the media. To avoid scanning the whole FAT of the file,
       we remember the current cluster position in the FS_FILE data structure.
    */
    if (prevclust == 0) {
      diskclustnum = pFile->CurClust;
      if (diskclustnum == 0) {
        /* No known current cluster position, we have to scan from the file's start cluster */
        diskclustnum = FS__fat_diskclust( pFile->FstClus, fileclustnum);
      }
    } 
    else {
      /* Get next cluster of the file starting at the current cluster */
      diskclustnum = FS__fat_diskclust( prevclust, 1);
    }
    prevclust        = diskclustnum;
    pFile->CurClust  = diskclustnum;
    if (diskclustnum == 0) {
      /* Translation to absolute cluster failed */
      FS__fat_free(buffer);
      return ((N * Size - todo) / Size);
    }
    diskclustnum -= 2;
    j = (pFile->Offset % bytesperclus) / FAT_SEC_SIZE;
    while (1) {  /* Cluster loop */
      if (!todo) {
        break;  /* Nothing more to write */
      }
      if (j >= FS__pDiskInfo[0].SecPerClus) {
        break; /* End of cluster reached */
      }
      i = pFile->Offset % FAT_SEC_SIZE;
      /* 
         We only have to read the sector from the media, if we do not
         modify the whole sector. That is the case if

         a) Writing starts not at the first byte of the sector
         b) Less data than the sector contains is written
      */
      lexp = (i != 0);
      lexp = lexp || (todo < FAT_SEC_SIZE);
      if (lexp) {
        /* We have to read the old sector */
        err = FS__lb_read(FS__pDevInfo[0].harddisk_driver,0, datastart +diskclustnum * FS__pDiskInfo[0].SecPerClus + j,
                                                                                                           (void*)buffer);
        if (err !=SD_NO_ERR) {
          FS__fat_free(buffer);
          return ((N * Size - todo) / Size);
        }
      }
      while (1) {  /* Sector loop */
        if (!todo) {
          break;  /* Nothing more to write */
        }
        if (i >= FAT_SEC_SIZE) {
          break;  /* End of sector reached */
        }
        buffer[i] = *((S8 *)(((S8 *)pData) + N * Size - todo));
        i++;
        pFile->Offset++;
        if (pFile->Offset > pFile->FileSize) {
          pFile->FileSize = pFile->Offset;
        }
        todo--;
      }  /* Sector loop */
      /* Write the modified sector */
      err = FS__lb_write(FS__pDevInfo[0].harddisk_driver, 0, datastart +diskclustnum * FS__pDiskInfo[0].SecPerClus + j,
                                                                                                             (void*)buffer);
      if (err!=SD_NO_ERR) {
        FS__fat_free(buffer);
        return ((N * Size - todo) / Size);
      }
      j++;
    }  /* Cluster loop */
  } /* Write data loop */
  if (i >= FAT_SEC_SIZE) {
    if (j >=  FS__pDiskInfo[0].SecPerClus) {
      /* File pointer is already in the next cluster */
      pFile->CurClust = FS__fat_diskclust(prevclust, 1);
    }
  }
  /* Modify directory entry */
  err = _FS_fat_read_dentry( pFile->FstClus, pFile->DirClus, &s, &dsec, buffer);
  if (err == 0) {
    FS__fat_free(buffer);
    return ((N * Size - todo) / Size);
  }
  s.data[28] = (S8)(pFile->FileSize & 0xff);   /* FileSize */
  s.data[29] = (S8)((pFile->FileSize / 0x100UL) & 0xff);   
  s.data[30] = (S8)((pFile->FileSize / 0x10000UL) & 0xff);
  s.data[31] = (S8)((pFile->FileSize / 0x1000000UL) & 0xff);
  err = _FS_fat_write_dentry( pFile->FstClus, &s, dsec, buffer);
  if (err == 0) {
   return FS_ERR;
  }
  FS__fat_free(buffer);
  return ((N * Size - todo) / Size);
}



/*********************************************************************
*
*             FS__fat_fclose
*
  Description:
  FS internal function. Close a file referred by pFile.

  Parameters:
  pFile       - Pointer to a FS_FILE data structure. 
  
  Return value:
  None.
*/

S32 FS__fat_fclose(FS_FILE *pFile) {
  FS__fat_dentry_type s;
  S8 *buffer;
  U32 dsec;
  S32 err;

  if (!pFile) {
      return FS_ERR;
  }
  /* Check if media is OK */
  /* Modify directory entry */
  buffer = (S8 *)FS__fat_malloc(FAT_SEC_SIZE);
  if ((S32)buffer==BUFFER_ERR) {
    pFile->inuse = 0;
    return FS_ERR;
  }
  err = _FS_fat_read_dentry( pFile->FstClus, pFile->DirClus, &s, &dsec, buffer);
  if (err == 0) {
    pFile->inuse = 0;
    FS__fat_free(buffer);
    return FS_ERR;
  }
  s.data[28] = (S8)(pFile->FileSize & 0xff);   /* FileSize */
  s.data[29] = (S8)((pFile->FileSize / 0x100UL) & 0xff);   
  s.data[30] = (S8)((pFile->FileSize / 0x10000UL) & 0xff);
  s.data[31] = (S8)((pFile->FileSize / 0x1000000UL) & 0xff);
  err = _FS_fat_write_dentry( pFile->FstClus, &s, dsec, buffer);
  if(err==0){
    pFile->inuse=0;
    return FS_ERR;
  }
  FS__fat_free(buffer);
  pFile->inuse = 0;
  return FS_NO_ERR;
}


