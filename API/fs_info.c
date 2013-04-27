#include "sdi.h"
#include "fs_int.h"
#include "def.h"
#include "fs_fat.h"

#define CACHE_NUM   (sizeof(_FS_CACHE_BUFFER)/sizeof(FS__CACHE_BUFFER))

extern SDFSDisk SanDisk_info_table;
extern const SDDriver  SanDisk_driver_table;
extern const FS__fsl_type FS__fat_functable;



FS__CACHE_BUFFER _FS_CACHE_BUFFER[]={{0,-1,0,},{0,-1,0,},{0,-1,0,},{0,-1,0,},{0,-1,0,},{0,-1,0,},{0,-1,0,},{0,-1,0,},{0,-1,0,},{0,-1,0,}};


HardDisk   _FS_devinfo[]={{CACHE_NUM,CACHE_NUM,&SanDisk_info_table,&SanDisk_driver_table,&FS__fat_functable,_FS_CACHE_BUFFER}};
HardDisk   * const FS__pDevInfo=_FS_devinfo;
