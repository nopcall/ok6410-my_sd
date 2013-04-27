#include "def.h"
#include "fs_int.h"
#include "fs_fat.h"
#include "lbl.h"
#include "sdi.h"

U8  FS__lb_read(const  SDDriver *pDriver, U32 startsec, U32 Sector, void *pBuffer)
{
      if(pDriver->Hard__ReadBlock)
      {
           return( (pDriver->Hard__ReadBlock)((startsec+Sector),(U8 *)pBuffer));
      
      }
      return SD_ERR;
}


U8    FS__lb_write(const  SDDriver *pDriver, U32 startsec, U32 Sector, void *pBuffer)
{

	  if(pDriver->Hard__WriteBlock)
	  {
	       return((pDriver->Hard__WriteBlock)((startsec+Sector),(U8 *)pBuffer));
	     
	  }
	  return SD_ERR;

}

