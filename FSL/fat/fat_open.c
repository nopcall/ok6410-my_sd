#include "fs_fat.h"
#include "fs_clib.h"
#include "fs_api.h"
#include "lbl.h"
#include "sdi.h"
/*********************************************************************
*
*             Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*             _FS_fat_find_file
*
  Description:
  FS internal function. Find the file with name pFileName in directory
  DirStart. Copy its directory entry to pDirEntry.
  
  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
  pFileName   - File name. 
  pDirEntry   - Pointer to an FS__fat_dentry_type data structure.
  DirStart    - 1st cluster of the directory.
  DirSize     - Sector (not cluster) size of the directory.
 
  Return value:
  >=0         - File found. Value is the first cluster of the file.
  <0          - An error has occured.
*/
//在一层目录中寻找一文件，返回文件的开始簇，跟FS__fat_find_dir相似
//FS__fat_find_dir是寻找目录

static S32 _FS_fat_find_file( const S8 *pFileName,
                                    FS__fat_dentry_type *pDirEntry,
                                    U32 DirStart, U32 DirSize) {
  FS__fat_dentry_type *s;
  U32 i;
  U32 dsec;
  S32 len;
  S32 err; 
  S32 c;
  S8 *buffer;

  buffer = (S8 *)FS__fat_malloc(FAT_SEC_SIZE);
  if (BUFFER_ERR==(S32)buffer) {
    return BUFFER_ERR;
  }
  len = FS__CLIB_strlen(pFileName);
  if (len > 11) {
    len = 11;
  }
  /* Read directory */
  for (i = 0; i < DirSize; i++) {
    dsec = FS__fat_dir_realsec(DirStart, i);
    if (dsec == 0) {
      FS__fat_free(buffer);
      return FS_ERR;
    }
    err =  FS__lb_read(FS__pDevInfo[0].harddisk_driver,0,dsec, (void*)buffer);
    if (err!=SD_NO_ERR) {
      FS__fat_free(buffer);
      return FS_ERR;
    }
    s = (FS__fat_dentry_type*)buffer;
    while (1) {
      if (s >= (FS__fat_dentry_type*)(buffer + FAT_SEC_SIZE)) {
        break;  /* End of sector reached */
      }
      c = FS__CLIB_strncmp((char*)s->data, pFileName, len);
      if (c == 0) {  /* Name does match */
        if (s->data[11] & FS_FAT_ATTR_ARCHIVE) {
          break;  /* Entry found */
        }
      }
      s++;
    }
    if (s < (FS__fat_dentry_type*)(buffer + FAT_SEC_SIZE)) {
      /* Entry found. Return number of 1st block of the file */
      if (pDirEntry) {
        FS__CLIB_memcpy(pDirEntry, s, sizeof(FS__fat_dentry_type));
      }
      FS__fat_free(buffer);
      dsec  = (U32)s->data[26];
      dsec += (U32)s->data[27] * 0x100UL;
      dsec += (U32)s->data[20] * 0x10000UL;
      dsec += (U32)s->data[21] * 0x1000000UL;
      return ((S32)dsec);
    }
  }
  FS__fat_free(buffer);
  return FS_ERR;
}


/*********************************************************************
*
*             _FS_fat_IncDir
*
  Description:
  FS internal function. Increase directory starting at DirStart.
  
  Parameters:
  DirStart    - 1st cluster of the directory.
  pDirSize    - Pointer to an FS_u32, which is used to return the new 
                sector (not cluster) size of the directory.
 
  Return value:
  ==1         - Success.
  ==-1        - An error has occured.
*/
//旧目录层已满尝试着扩展旧目录层
static S32 _FS_fat_IncDir( U32 DirStart, U32 *pDirSize) {
  U32 i;
  U32 dsec;
  S32 last;
  S8 *buffer;
  S32 err;

  if (DirStart == 0) { 
    /* Increase root directory only, if not FAT12/16  */
                                              //FAT12/16根目录已满，无法扩展，返回错误
    if (FS__pDiskInfo[0].FATType==FAT16||FS__pDiskInfo[0].FATType==FAT12) {                            
      return FS_ERR;  /* Not FAT32 */              //FAT32的根目录也是一个文件所以根目录可以扩展
    }
  }
  last = FS__fat_FAT_find_eof(DirStart, 0);
  if (last !=FS_NO_ERR) {
    return FS_ERR;  /* No EOF marker found */
  }
  last = FS__fat_FAT_alloc(last);  /* Allocate new cluster */
  if (last !=FS_NO_ERR) {
    return FS_ERR;
  }
  *pDirSize = *pDirSize + FS__pDiskInfo[0].SecPerClus;
  /* Clean new directory cluster */
  buffer =(S8 *) FS__fat_malloc(FAT_SEC_SIZE);
  if ((S32)buffer==BUFFER_ERR) {
    return FS_ERR;
  }
  FS__CLIB_memset(buffer, 0x00, (U32)FAT_SEC_SIZE);
  for (i = *pDirSize - FS__pDiskInfo[0].SecPerClus; i < *pDirSize; i++) {
    dsec = FS__fat_dir_realsec(DirStart, i);
    if (dsec == 0) {
      FS__fat_free(buffer);
      return FS_ERR;
    }
    err =  FS__lb_write(FS__pDevInfo[0].harddisk_driver, 0, dsec, (void*)buffer);
    if (err!=SD_NO_ERR) {
      FS__fat_free(buffer);
      return FS_ERR;
    }
  }
  FS__fat_free(buffer);
  return FS_NO_ERR;
}


/*********************************************************************
*
*             _FS_fat_create_file
*
  Description:
  FS internal function. Create a file in the directory specified
  with DirStart. Do not call, if you have not checked before for 
  existing file with name pFileName.

  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number, which is passed to the device driver.
  pFileName   - File name. 
  DirStart    - Start of directory, where to create pDirName.
  DirSize     - Sector size of the directory starting at DirStart.
  
  Return value:
  >=0         - 1st cluster of the new file.
  ==-1        - An error has occured.
  ==-2        - Cannot create, because directory is full.
*/

static S32 _FS_fat_create_file(  const S8 *pFileName,
                                    U32 DirStart, U32 DirSize) {
  FS__fat_dentry_type *s;
  U32 i;
  U32 dsec;
  S32 cluster;
  S32 len;
  S32 err;
  S8 *buffer;

  buffer = (S8 *)FS__fat_malloc(FAT_SEC_SIZE);
  if (BUFFER_ERR==(S32)buffer) {
    return FS_ERR;
  }
  len = FS__CLIB_strlen(pFileName);
  if (len > 11) {
    len = 11;
  }
  /* Read directory */
  for (i = 0; i < DirSize; i++) {
    dsec = FS__fat_dir_realsec( DirStart, i);
    if (dsec == 0) {
      FS__fat_free(buffer);
      return FS_ERR;
    }
    err = FS__lb_read(FS__pDevInfo[0].harddisk_driver,0, dsec, (void*)buffer);
    if (err!=SD_NO_ERR) {
      FS__fat_free(buffer);
      return FS_ERR;
    }
    s = (FS__fat_dentry_type*)buffer;
    while (1) {
      if (s >= (FS__fat_dentry_type*)(buffer + FAT_SEC_SIZE)) {
        break;  /* End of sector reached */
      }
      if (s->data[0] == 0x00) {
        break;  /* Empty entry found */
      }
      if (s->data[0] == (U8)0xe5) {
        break;  /* Deleted entry found */
      }
      s++;
    }
    if (s < (FS__fat_dentry_type*)(buffer + FAT_SEC_SIZE)) {
      /* Free entry found. Make entry and return 1st block of the file */
      FS__CLIB_strncpy((char*)s->data, pFileName, len);
      s->data[11] = FS_FAT_ATTR_ARCHIVE;
      /* Alloc block in FAT */
      cluster = FS__fat_FAT_alloc( -1);
      if (cluster >= 0) {
        s->data[12]     = 0x00;                           /* Res */
        s->data[13]     = 0x00;                           /* CrtTimeTenth (optional, not supported) */
        s->data[14]     = 0x00;                           /* CrtTime (optional, not supported) */
        s->data[15]     = 0x00;
        s->data[16]     = 0x00;                           /* CrtDate (optional, not supported) */
        s->data[17]     = 0x00;
        s->data[18]     = 0x00;                           /* LstAccDate (optional, not supported) */
        s->data[19]     = 0x00;
        s->data[22]     = 0x00;   /* WrtTime */
        s->data[23]     = 0x00;
        s->data[24]     = 0x00;   /* WrtDate */
        s->data[25]     = 0x00;
        s->data[26]     = (U8)(cluster & 0xff);    /* FstClusLo / FstClusHi */ 
        s->data[27]     = (U8)((cluster / 256) & 0xff);
        s->data[20]     = (U8)((cluster / 0x10000L) & 0xff);
        s->data[21]     = (U8)((cluster / 0x1000000L) & 0xff);
        s->data[28]     = 0x00;                           /* FileSize */
        s->data[29]     = 0x00;
        s->data[30]     = 0x00;
        s->data[31]     = 0x00;
        err = FS__lb_write(FS__pDevInfo[0].harddisk_driver,0, dsec, (void*)buffer);
        if (err!=SD_NO_ERR) {
          FS__fat_free(buffer);
          return FS_ERR;
        }
      }
      FS__fat_free(buffer);
      return cluster;
    }
  }
  FS__fat_free(buffer);
  return DIR_FULL;      /* Directory is full */
}

/*********************************************************************
*
*             Global functions section 1
*
**********************************************************************



/*********************************************************************
*
*             FS__fat_DeleteFileOrDir
*
  Description:
  FS internal function. Delete a file or directory.
  
  Parameters:
  pName       - File or directory name. 
  DirStart    - Start of directory, where to create pDirName.
  DirSize     - Sector size of the directory starting at DirStart.
  RmFile      - 1 => remove a file
                0 => remove a directory
  
  Return value:
  >=0         - Success. 
  <0          - An error has occured.
*/
//直接删除一个层上的文件或目录，此函数并未将目录下面的目录或文件一并删除，用时需注意
S32 FS__fat_DeleteFileOrDir(  S8 *pName, U32 DirStart, U32 DirSize, S8 RmFile) {
  FS__fat_dentry_type *s;
  U32 dsec;
  U32 i;
  U32 value;
  U32 fatsize;
  U32 filesize;
  S32 len;
  S32 bytespersec;
  S32 fatindex;
  S32 fatsec;
  S32 fatoffs;
  S32 lastsec;
  S32 curclst;
  S32 todo;
  S8 *buffer;
  S32 fattype;
  S32 err;
  S32 err2;
  S32 lexp;
  S32 x;
  U8 a;
  U8 b;
  buffer = (S8 *)FS__fat_malloc(FAT_SEC_SIZE);
  if ((S32)buffer==BUFFER_ERR) {
    return FS_ERR;
  }
  fattype = FS__fat_which_type();
  fatsize = FS__pDiskInfo[0].FATSecCnt;
  bytespersec = (S32)FS__pDiskInfo[0].BytsPerSec;
  len = FS__CLIB_strlen(pName);
  if (len > 11) {
    len = 11;
  }
  /* Read directory */
  for (i = 0; i < DirSize; i++) {
    curclst = -1;
    dsec = FS__fat_dir_realsec( DirStart, i);
    if (dsec == 0) {
      FS__fat_free(buffer);
      return FS_ERR;
    }
    err =  FS__lb_read(FS__pDevInfo[0].harddisk_driver, 0, dsec, (void*)buffer);
    if (err !=SD_NO_ERR) {
      FS__fat_free(buffer);
      return FS_ERR;
    }
    /* Scan for pName in the directory sector */
    s = (FS__fat_dentry_type*) buffer;
    while (1) {
      if (s >= (FS__fat_dentry_type*)(buffer + bytespersec)) {
        break;  /* End of sector reached */
      }
      x = FS__CLIB_strncmp((char*)s->data, pName, len);
      if (x == 0) { /* Name does match */
        if (s->data[11] != 0) {
          break;  /* Entry found */
        }
      }
      s++;
    }
    if (s < (FS__fat_dentry_type*)(buffer + bytespersec)) {
      /* Entry has been found, delete directory entry */
      s->data[0]  = 0xe5;
      s->data[11] = 0;
      err = FS__lb_write(FS__pDevInfo[0].harddisk_driver, 0, dsec, (void*)buffer);
      if (err != SD_NO_ERR) {
        FS__fat_free(buffer);
        return FS_ERR;
      }
      /* Free blocks in FAT */
      /*
         For normal files, there are no more clusters freed than the entrie's filesize
         does indicate. That avoids corruption of the complete media in case there is
         no EOF mark found for the file (FAT is corrupt!!!). 
         If the function should remove a directory, filesize if always 0 and cannot
         be used for that purpose. To avoid running into endless loop, todo is set
         to 0x0ffffff8L, which is the maximum number of clusters for FAT32.
      */
      if (RmFile) {
        filesize  = s->data[28] + 0x100UL * s->data[29] + 0x10000UL * s->data[30] + 0x1000000UL * s->data[31];
        todo      = filesize / (FS__pDiskInfo[0].SecPerClus * bytespersec);
        value     = filesize % (FS__pDiskInfo[0].SecPerClus * bytespersec);
        if (value != 0) {
          todo++;
        }
      } 
      else {
        todo = (S32)0x0ffffff8L;
      }
      curclst = s->data[26] + 0x100L * s->data[27] + 0x10000L * s->data[20] + 0x1000000L * s->data[21];
      lastsec = -1;
      /* Free cluster loop */
      while (todo) {
        if(fattype==FAT16) {
          fatindex = curclst * 2;               /* FAT16 */
        }
        fatsec = FS__pDiskInfo[0].FATStartSec + (fatindex / bytespersec);
        fatoffs = fatindex % bytespersec;
        if (fatsec != lastsec) {
          err = FS__lb_read(FS__pDevInfo[0].harddisk_driver, 0, fatsec, (void*)buffer);
          if (err !=SD_NO_ERR) {
            err =FS__lb_read(FS__pDevInfo[0].harddisk_driver, 0, fatsize + fatsec, (void*)buffer);
            if (err !=SD_NO_ERR) {
              FS__fat_free(buffer);
              return FS_ERR;
            }
            /* Try to repair original FAT sector with contents of copy */
             FS__lb_write(FS__pDevInfo[0].harddisk_driver, 0, fatsec, (void*)buffer);
          }
          lastsec = fatsec;
        }
        if(fattype==FAT16) {
          a = buffer[fatoffs];
          b = buffer[fatoffs + 1];
          buffer[fatoffs]     = 0x00;
          buffer[fatoffs + 1] = 0x00;
          err  =  FS__lb_write(FS__pDevInfo[0].harddisk_driver,0, fatsec, (void*)buffer);
          err2 =  FS__lb_write(FS__pDevInfo[0].harddisk_driver,0, fatsize + fatsec, (void*)buffer);
          lexp = (err!=SD_NO_ERR);
          lexp = lexp || (err2!=SD_NO_ERR);
          if (lexp) {
            FS__fat_free(buffer);
            return FS_ERR;
          }
          curclst  = a + 256 * b;
          curclst &= 0xffff;
          if (curclst >= (S32)0xfff8) {
            FS__fat_free(buffer);
            return 0;
          }
        }
        todo--;
      } /* Free cluster loop */
    } /*  Delete entry */
    if (curclst > 0) {
      FS__fat_free(buffer);
      return curclst;
    }
  } /* for */
  FS__fat_free(buffer);
  return curclst;
}


/*********************************************************************
*
*             FS__fat_make_realname
*
  Description:
  FS internal function. Convert a given name to the format, which is
  used in the FAT directory.
  
  Parameters:
  pOrgName    - Pointer to name to be translated
  pEntryName  - Pointer to a buffer for storing the real name used
                in a directory.

  Return value:
  None.
*/
//将长度为a[12]的文件名转化成FAT采用的文件名,其中名字末尾是'\0'
void FS__fat_make_realname(S8 *pEntryName, const S8 *pOrgName) {
  S8 *ext;
  S8 *s;
  S32 i;

  s = (S8 *)pOrgName;
  ext = (S8 *) FS__CLIB_strchr(s, '.');//ext指向'.'
  if (!ext) {
    ext = &s[FS__CLIB_strlen(s)];//如果没有'.'   ext->'\0'
  }
  i=0;
  while (1) {
    if (s >= ext) {
      break;  /* '.' reached */
    }
    if (i >= 8) {
      break;  /* If there is no '.', this is the end of the name */
    }
    if (*s == (char)0xe5) {
      pEntryName[i] = 0x05;
    }
    else {
      pEntryName[i] = (char)FS__CLIB_toupper(*s);
    }
    i++;
    s++;
  }
  while (i < 8) {
    /* Fill name with spaces*/
    pEntryName[i] = ' ';
    i++;
  }
  if (*s == '.') {
    s++;
  }
  while (i < 11) {
    if (*s != '\0') {
      if (*s == (char)0xe5) {
        pEntryName[i] = 0x05;
      }
      else {
        pEntryName[i] = (char)FS__CLIB_toupper(*s);
      }
      s++;
    }
    else {
      pEntryName[i] = ' ';
    }
    i++;
  }
  pEntryName[11]='\0';
}

/*********************************************************************
*
*             FS__fat_dirsize
*
  Description:
  FS internal function. Return the sector size of the directory 
  starting at DirStart.
  
  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
  DirStart    - 1st cluster of the directory. This is zero to address 
                the root directory. 
 
  Return value:
  >0          - Sector (not cluster) size of the directory.
  ==0         - An error has occured.
*/
//找出一层目录包含的扇区数

U32 FS__fat_dir_size( U32 DirStart) {
  U32 dsize;
  S32 value;

  if (DirStart == 0) {
    /* For FAT12/FAT16 root directory, the size can be found in BPB */
    dsize = FS__pDiskInfo[0].RootSecCnt;
  }
  else {
    /* Calc size of a sub-dir */
    value = FS__fat_FAT_find_eof( DirStart, &dsize);
    if (value < 0) {
      dsize = 0;
    }
    else {
      dsize *= FS__pDiskInfo[0].SecPerClus;
    }
  }
  return dsize;
}


/*********************************************************************
*
*             FS__fat_dir_realsec
*
  Description:
  FS internal function. Translate a directory relative sector number
  to a real sector number on the media.
  
  Parameters:
  DirStart    - 1st cluster of the directory. This is zero to address 
                the root directory. 
  DirSec      - Sector in the directory.
 
  Return value:
  >0          - Directory found. Value is the sector number on the media.
  ==0         - An error has occured.
*/
//返回相对于DirStart簇的扇区的真实扇区号

U32 FS__fat_dir_realsec( U32 DirStart, U32 DirSec) {
  U32 rootdir;
  U32 rsec;
  U32 dclust;
  U32 fatsize;
  S32 fattype;
  S32 lexp;
  U8 secperclus;

  fattype = FS__fat_which_type();
  lexp = (0 == DirStart);
  lexp = lexp && (fattype==FAT16);
  if (lexp) {
    /* Sector in FAT12/FAT16 root directory */
    rootdir =FS__pDiskInfo[0].RootDirTable;
    rsec = rootdir + DirSec;
  }
  else {
    fatsize = FS__pDiskInfo[0].FATSecCnt;
    secperclus = FS__pDiskInfo[0].SecPerClus;
    dclust = DirSec / secperclus;
    rsec = FS__fat_diskclust( DirStart, dclust);
    if (rsec == 0) {
      return 0;
    }
    rsec -= 2;
    rsec *= secperclus;
    rsec+=FS__pDiskInfo[0].DataStartSec;
    rsec += (DirSec % secperclus);
  }
  return rsec;
}


/*********************************************************************
*
*             FS__fat_find_dir
*
  Description:
  FS internal function. Find the directory with name pDirName in directory
  DirStart.
  
  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
  pDirName    - Directory name; if zero, return the root directory.
  DirStart    - 1st cluster of the directory.
  DirSize     - Sector (not cluster) size of the directory.
 
  Return value:
  >0          - Directory found. Value is the first cluster of the file.
  ==0         - An error has occured.
*/
//在一层目录树中寻找目录返回一个目录的开始簇号
U32 FS__fat_find_dir( S8 *pDirName, U32 DirStart,U32 DirSize) {
  FS__fat_dentry_type *s;
  U32 dstart;
  U32 i;
  U32 dsec;
  S32 len;
  S32 err;
  S32 c;
  S8 *buffer;

  if (pDirName == 0) {
    /* Return root directory */
      dstart=FS__pDiskInfo[0].RootDirTable;
  }
  else {
    /* Find directory */
    buffer = (S8 *)FS__fat_malloc(FAT_SEC_SIZE);
    if (BUFFER_ERR==(S32)buffer) {
      return 0;
    }
    len = FS__CLIB_strlen(pDirName);
    if (len > 11) {
      len = 11;
    }
    /* Read directory */
    for (i = 0; i < DirSize; i++) {
      dsec = FS__fat_dir_realsec(DirStart, i);
      if (dsec == 0) {
        FS__fat_free(buffer);
        return 0;
      }
      err = FS__lb_read(FS__pDevInfo[0].harddisk_driver, 0, dsec, (void*)buffer);
      if (err!=SD_NO_ERR) {
        FS__fat_free(buffer);
        return 0;
      }
      s = (FS__fat_dentry_type*)buffer;
      while (1) {
        if (s >= (FS__fat_dentry_type*)(buffer + FAT_SEC_SIZE)) {
          break;  /* End of sector reached */
        }
        c = FS__CLIB_strncmp((char*)s->data, pDirName, len);
        if (c == 0) { /* Name does match */
          if (s->data[11] & FS_FAT_ATTR_DIRECTORY) {
            break;  /* Entry found */
          }
        }
        s++;
      }
      if (s < (FS__fat_dentry_type*)(buffer + FAT_SEC_SIZE)) {
        /* Entry found. Return number of 1st block of the directory */
        FS__fat_free(buffer);
        dstart  = (U32)s->data[26];
        dstart += (U32)0x100UL * s->data[27];
        dstart += (U32)0x10000UL * s->data[20];
        dstart += (U32)0x1000000UL * s->data[21];
        return dstart;
      }
    }
    dstart = 0;
    FS__fat_free(buffer);
  }
  return dstart;
}



/*********************************************************************
*
*             FS__fat_findpath
*
  Description:
  FS internal function. Return start cluster and size of the directory
  of the file name in pFileName.
  
  Parameters:
  pFullName   - Fully qualified file name w/o device name.
  pFileName   - Pointer to a pointer, which is modified to point to the
                file name part of pFullName.
  pUnit       - Pointer to an FS_u32 for returning the unit number.
  pDirStart   - Pointer to an FS_u32 for returning the start cluster of
                the directory.

  Return value:
  >0          - Sector (not cluster) size of the directory.
  ==0         - An error has occured.
*/

//根据路径寻找一个目录文件，返回目录文件的开始簇号，目录文件的路径必须是   \\***\\****\\*****\\

U32 FS__fat_findpath( const S8 *pFullName, S8 **pFileName,U32 *pDirStart) {
  U32 dsize;
  S32 i;
  S32 j;
  S8 *dname_start;
  S8 *dname_stop;
  S8 *chprt;
  S32 x;
  S8 dname[12];
  S8 realname[12];
    /* Use 1st unit as default */
    *pFileName = (S8 *) pFullName;
  /* Check volume */
  x = FS__fat_checkunit();
  if (x!=FS_NO_ERR) {
    return 0;
  }
  /* Setup pDirStart/dsize for root directory */
  *pDirStart = 0;
  dsize      = FS__fat_dir_size(0);
  /* Find correct directory */
  do {
    dname_start = (S8 *)FS__CLIB_strchr(*pFileName, '\\');
    if (dname_start) {
      dname_start++;
      *pFileName = dname_start;
      dname_stop = (S8 *)FS__CLIB_strchr(dname_start, '\\');
    }
    else {
      dname_stop = 0;
    }
    if (dname_stop) {
      i = dname_stop-dname_start;
      if (i >= 12) {
        j = 0;
        for (chprt = dname_start; chprt < dname_stop; chprt++) {
          if (*chprt == '.') {
            i--;
          }
          else if (j < 12) {
            realname[j] = *chprt;
            j++;
          }
        }
        if (i >= 12) {
          return 0;
        }
      }
      else {
        FS__CLIB_strncpy(realname, dname_start, i);
      }
      realname[i] = '\0';
      FS__fat_make_realname(dname, realname);
      *pDirStart =  FS__fat_find_dir(dname, *pDirStart, dsize);
      if (*pDirStart) {
        dsize  =  FS__fat_dir_size(*pDirStart);
      }
      else {
        dsize = 0;    /* Directory NOT found */
      }
    }
  } while (dname_start);
  return dsize;
}



/*********************************************************************
*
*             Global functions section 2
*
**********************************************************************

  These are real global functions, which are used by the API Layer
  of the file system.
  
*/

/*********************************************************************
*
*             FS__fat_fopen
*
  Description:
  FS internal function. Open an existing file or create a new one.

  Parameters:
  pFileName   - File name. 
  pMode       - Mode for opening the file.
  pFile       - Pointer to an FS_FILE data structure.
  
  Return value:
  ==0         - Unable to open the file.
  !=0         - Address of the FS_FILE data structure.
*/

FS_FILE *FS__fat_fopen(S8 *pFileName,const S8 *pMode, FS_FILE *pFile) {
  U32 dstart;
  U32 dsize;
  S32 i;
  S8 * fname;
  FS__fat_dentry_type s;
  S8 realname[12];
  S32 lexp_a;
  S32 lexp_b;
  S32 lexp_c;
  
  if (!pFile) {
    return 0;  /* Not a valid pointer to an FS_FILE structure*/
  }
  dsize = FS__fat_findpath( pFileName, &fname, &dstart);
  if (dsize == 0) {
    return 0;  /* Directory not found */
  }
  FS__fat_make_realname(realname, fname);  /* Convert name to FAT real name */
  /* FileSize = 0 */
  s.data[28] = 0x00;      
  s.data[29] = 0x00;
  s.data[30] = 0x00;
  s.data[31] = 0x00;
  i = _FS_fat_find_file( realname, &s, dstart, dsize);
  if(i==0){
    FS__fat_DeleteFileOrDir(realname,dstart,dsize,1);
    i=_FS_fat_create_file(realname,dstart,dsize);
  }
  lexp_c=(i>0);
  /* Delete file */
  lexp_b = (FS__CLIB_strcmp(pMode, "del") == 0);    /* Delete file request */
  lexp_a = lexp_b && (i >= 0);                      /* File does exist */
  if (lexp_a) {
    i = FS__fat_DeleteFileOrDir( realname, dstart, dsize, 1);
    if (i < 0) {
      return (FS_FILE *)FS_ERR;
    }
    return 0;
  }
  else if (lexp_b) {
    return 0;
  }
  /* Check read only */
  lexp_a = ((i >= 0) && ((s.data[11] & FS_FAT_ATTR_READ_ONLY) != 0)) &&((pFile->mode_w) || (pFile->mode_a) || (pFile->mode_c));
  if (lexp_a) {
    /* Files is RO and we try to create, write or append */
    return 0;
  }
  lexp_a = (( i>= 0) && (!pFile->mode_a) && (((pFile->mode_w) && (!pFile->mode_r)) || ((pFile->mode_w) && (pFile->mode_c) && (pFile->mode_r)) ));
  if (lexp_a) {
    /* Delete old file */
    i = FS__fat_DeleteFileOrDir( realname, dstart, dsize, 1);
    /* FileSize = 0 */
    s.data[28] = 0x00;      
    s.data[29] = 0x00;
    s.data[30] = 0x00;
    s.data[31] = 0x00;
    i=-1;
  }
  if ((!pFile->mode_c) && (i < 0)) {
    /* File does not exist and we must not create */
    return 0;
  }
  else if ((pFile->mode_c) && (i < 0)) {
    /* Create new file */
    i = _FS_fat_create_file(realname, dstart, dsize);
    if (i < 0) {
      /* Could not create file */
      if (i == DIR_FULL) {
        /* Directory is full, try to increase */
        i = _FS_fat_IncDir( dstart, &dsize);
        if (i ==FS_NO_ERR) {
          i = _FS_fat_create_file(realname, dstart, dsize);
        }
      }
      if (i != FS_NO_ERR) {
        return 0;
      }
    }
  }
  pFile->CurClust   = 0;
  pFile->FstClus    = i;
  pFile->DirClus    =dstart;
  pFile->EOFClust   = -1;
  pFile->CacheId    =-1;
  pFile->dirty      =0;
  pFile->FileSize       = (U32)s.data[28];   /* FileSize */
  pFile->FileSize      += (U32)0x100UL * s.data[29];
  pFile->FileSize      += (U32)0x10000UL * s.data[30];
  pFile->FileSize      += (U32)0x1000000UL * s.data[31];
  if (pFile->mode_a) {
    pFile->Offset   = pFile->FileSize;
  }
  else {
    pFile->Offset   = 0;
  }
  pFile->inuse      = 1;
  return pFile;
}


