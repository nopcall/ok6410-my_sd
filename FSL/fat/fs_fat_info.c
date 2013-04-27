#include "def.h"
#include "fs_fat.h"
#include "lbl.h"
#include <stdio.h>


Disk_Info     _pDisk_Info[]={
          {
          0,
          0xff,
          0,
          0,
          0,
          0xffffffff,
          0,
          0,
          0,
          0,
          0,
          0,
          0,
          0,
          NULL,
          }
          
          

};

Disk_Info *const FS__pDiskInfo=_pDisk_Info;

