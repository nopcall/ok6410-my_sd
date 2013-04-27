#ifndef _LBL_H_
#define _LBL_H_
#include "def.h"
#include "fs_int.h"
#include "fs_fat.h"

U8                 FS__lb_status(const  SDDriver *pDriver, U32 Unit);
U8                 FS__lb_read(const  SDDriver*pDriver, U32 startsec, U32 Sector, void *pBuffer);
U8                 FS__lb_write(const  SDDriver *pDriver, U32 startsec, U32 Sector, void *pBuffer);
U8                 FS__lb_ioctl(const  SDDriver *pDriver, U32 Unit,U16 Cmd, U32 Aux, void *pBuffer);



#endif