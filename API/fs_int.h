#ifndef _FS_INT_H_
#define _FS_INT_H_
#include "def.h"
#include "fs_api.h"
#include "fs_fat.h"


#define CACHE_BUFFER_SIZE   (0x10*FAT_SEC_SIZE)
typedef struct _SD_DISK
{
    U16 RCA;
    U8  card_type;
    U32 block_num;
    U32 block_len;
    U32 sector_size;
    U32 timeout_read;
    U32 timeout_write;
    U32 timeout_erase;
}SDFSDisk;
typedef struct{
   U8              (*Hard__ReadBlock)(U32 blcoknum,U8 *recbuf);
   U8              (*Hard__WriteBlock)(U32 blocknum,U8 *recbuf);
   U8              (*Hard__EraseSector)(const SDFSDisk *sds,U32 sectornum);
   U8              (*Hard__Ioctl)(SDFSDisk *sds,U32 cmd,U8 flag,U32 blocknum,U8 *recbuf);
}SDDriver;

typedef struct {
  S8                  *name;
  FS_FILE *           (*fsl_fopen)(S8 *pFileName, const S8 *pMode, FS_FILE *pFile);
  S32                 (*fsl_fclose)(FS_FILE *pFile);
  U32                 (*fsl_fread)(void *pData, U32 Size, U32 N, FS_FILE *pFile);
  S32                 (*fsl_fwrite)(const void *pData,U32 Size, U32 N, FS_FILE *pFile);
  S32                 (*fsl_ftell)(FS_FILE *pFile);
  S32                 (*fsl_fseek)(FS_FILE *pFile, S32 Offset, S32 Whence);
  S32                 (*fsl_ioctl)(S32 Idx, U32 Id, S32 Cmd, S32 Aux, void *pBuffer);
} FS__fsl_type;



typedef struct {
  FS_FILE *handle;
  S32 fileOldOffset;
  U32 CacheOffset;
  S8  SecsDatabuff[CACHE_BUFFER_SIZE];
}FS__CACHE_BUFFER;


typedef struct
{
   const S32  maxcache;
   S32  freecache;
   SDFSDisk *const harddisk_info;
   const SDDriver *const harddisk_driver;
   const FS__fsl_type *const fs_ptr;
   FS__CACHE_BUFFER  *const pDevCacheInfo;
}HardDisk;




extern  HardDisk         *const FS__pDevInfo;


#endif