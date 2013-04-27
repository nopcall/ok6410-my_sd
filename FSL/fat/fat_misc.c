#include "fs_fat.h"
#include "fs_clib.h"
#include "lbl.h"
#include "sdi.h"


#define FS_MEMBLOCK_NUM  (FS_MAXOPEN+FS_DIR_MAXOPEN)*2

typedef    struct{
 
      U8 status;
      U8 memory[FAT_SEC_SIZE];
}_FS_FAT_block_type;

static  _FS_FAT_block_type  _FS_memblock[FS_MEMBLOCK_NUM];

void    FS__fat_block_init(void)
{
        U32 i;
        for(i=0;i<FS_MEMBLOCK_NUM;i++)
        {
        	_FS_memblock[i].status=0;
        }
}

U8      *FS__fat_malloc(U32 Size)
{
		U32 i;
		if(Size<=FAT_SEC_SIZE)
		{
			for(i=0;i<FS_MEMBLOCK_NUM;i++)
			{
				if(_FS_memblock[i].status==0)
				{
					_FS_memblock[i].status=1;
					FS__CLIB_memset((void *)_FS_memblock[i].memory,0,(U32)FAT_SEC_SIZE);
					return (_FS_memblock[i].memory);
				}
			}
		}
		return (U8 *)BUFFER_ERR;
}

void    FS__fat_free(void *pBuffer)
{
		U32 i;
		for(i=0;i<FS_MEMBLOCK_NUM;i++)
		{
			if(((void *)_FS_memblock[i].memory)==pBuffer)
			{
				_FS_memblock[i].status=0;
			}
		}
}

static S32 _FS_ReadBPB(void) {
  U8 *buffer;
  buffer = FS__fat_malloc(FAT_SEC_SIZE);
  if ((S32)BUFFER_ERR==(S32)buffer) {
    return BUFFER_ERR;
  }
 
	if(FS__lb_read(FS__pDevInfo[0].harddisk_driver,0,0,(void *)buffer)!=SD_NO_ERR)
  	{
  	   FS__fat_free(buffer);
  	   return FS_ERR;
  	}
  		    

  if((buffer[FAT_SEC_SIZE-2]+ 256 * buffer[FAT_SEC_SIZE-1])!=0xaa55)
  {
      FS__fat_free(buffer);
      return BPB_ERR;
  }
  if(buffer[0]!=0xeb)
  {
        FS__pDiskInfo[0].StartSec=((buffer[0x01c9]<<24)+(buffer[0x01c8]<<16)+(buffer[0x01c7]<<8)+buffer[0x01c6]);
        if(FS__lb_read(FS__pDevInfo[0].harddisk_driver,FS__pDiskInfo[0].StartSec,0,(void *)buffer)!=SD_NO_ERR)
  		{
  		    FS__pDiskInfo[0].StartSec=0;
  		    FS__fat_free(buffer);
  		    return FS_ERR;
  		}
             
  }
  else
  {
        FS__pDiskInfo[0].StartSec=0;
  }
  if(buffer[0]!=0xeb)
  {
      FS__fat_free(buffer);
      return BPB_ERR;
  }    
  FS__pDiskInfo[0].SecPerClus   =buffer[0x0d];
  FS__pDiskInfo[0].NumFATs      =buffer[0x10];
  FS__pDiskInfo[0].SecPerDisk   =(buffer[0x13]+(buffer[0x14]<<8));
  if(FS__pDiskInfo[0].SecPerDisk==0)
  {
  		FS__pDiskInfo[0].SecPerDisk=(buffer[0x20]+(buffer[0x21]<<8)+(buffer[0x22]<<16)+(buffer[0x23]<<24));
  }
  FS__pDiskInfo[0].BytsPerSec  = (buffer[0x0b]+(buffer[0x0c]<<8));
  FS__pDiskInfo[0].FATSecCnt   = (buffer[0x16]+(buffer[0x17]<<8));
  FS__pDiskInfo[0].FATStartSec =FS__pDiskInfo[0].StartSec+(buffer[0x0e]+(buffer[0x0f]<<8));
  FS__pDiskInfo[0].RootDirTable=FS__pDiskInfo[0].FATStartSec +FS__pDiskInfo[0].FATSecCnt*FS__pDiskInfo[0].NumFATs;
  FS__pDiskInfo[0].RootSecCnt  =((buffer[0x11]+(buffer[12]<<8))*0x20)/0x200;
  FS__pDiskInfo[0].DataStartSec=FS__pDiskInfo[0].RootDirTable+FS__pDiskInfo[0].RootSecCnt;
  FS__pDiskInfo[0].ClusPerData=(FS__pDiskInfo[0].SecPerDisk-(FS__pDiskInfo[0].DataStartSec-FS__pDiskInfo[0].StartSec))/FS__pDiskInfo[0].SecPerClus;
  FS__pDiskInfo[0].FATType    =FS__fat_which_type();
  FS__pDiskInfo[0].Drive_unit=1;
  FS__fat_free(buffer);
 return FS_NO_ERR;
}

U8   FS__fat_which_type(void)
{
     if(FS__pDiskInfo[0].ClusPerData<(0xfff-0xa))
     {
     		return FAT12;
     }
     else if(FS__pDiskInfo[0].ClusPerData<(0xffff-0xa))
     {
     		return FAT16;
     }
     return FAT32;
}



/*********************************************************************
*
*             _FS__fat_FindFreeCluster
*
  Description:
  FS internal function. Find the next free entry in the FAT.

  Parameters:
  pFATSector  - 空闲簇所在的FAT扇区号. 
  pLastSector - Returns the sector number of the sector in pBuffer.
  pFATOffset  - Returns the offset of the free FAT entry within the
                sector pFATSector.
  LastClust   - Cluster, which will be used to link the new allocated
                cluster to. Here it is used at hint for where to start
                in the FAT.
  pBuffer     - Pointer to a sector buffer.
  FSysType    - ==1 => FAT12
                ==0 => FAT16
                ==2 => FAT32
  FATSize     - Size of one FAT ind sectors.
  BytesPerSec - Number of bytes in each sector.
 
  Return value:
  >=0         - Number of the free cluster.
  <0          - An error has occured.
*/


static S32 _FS__fat_FindFreeCluster( U32 *pFATSector, S32 *pLastSector, U32 *pFATOffset, 
                                       S32 LastClust, U8 *pBuffer,  U8 FAT_type,   U32 FATSize, U32 BytesPerSec) {
  U32 totclst;
  U32 curclst;
  U32 fatindex;
  int scan;
  U8 fatentry;//簇使用标志!=0说明非空闲
  U8 a;
  U8 b;
  
  if (LastClust >= 2) {
    curclst = LastClust + 1;  /* Start scan after the previous allocated sector */
  }
  else {
    curclst = 2;  /*  Start scan at the beginning of the media */
  }
  scan          = 0;
  *pFATSector   = 0;                    //计算curclst所在的FAT表开始的扇区
  *pLastSector  = -1;                    //当前缓冲区中的扇区号
  fatentry      = 0xff;                 //簇使用标志!=0说明非空闲
  /* Calculate total number of data clusters on the media */
  totclst = (FS__pDiskInfo[0].ClusPerData);
  while (1) {
    if (curclst >= totclst) {
      scan++;
      if (scan > 1) {
        break;  /* End of clusters reached after 2nd scan */
      }
      if (LastClust <= 2) {
        break;  /* 1st scan started already at zero */
      }
      curclst   = 2;  /* Try again starting at the beginning of the FAT */
      fatentry  = 0xff;
    }
    if (fatentry == 0) {
      break;  /* Free entry found */
    }
     if(FAT_type==FAT16)
     {
        fatindex = curclst * 2; /* FAT16 */
     }              
    *pFATSector = FS__pDiskInfo[0].FATStartSec + (fatindex / BytesPerSec);//计算curclst所在的FAT表开始的扇区
    *pFATOffset = fatindex % BytesPerSec;
    
    
    if (*pFATSector != *pLastSector) {//查看所需的FAT是否存在于缓冲区中
        if(FS__lb_read(FS__pDevInfo[0].harddisk_driver,0,(*pFATSector),(void *)pBuffer)!=SD_NO_ERR)
        {
         if(FS__lb_read(FS__pDevInfo[0].harddisk_driver,0,((*pFATSector)+FS__pDiskInfo[0].FATSecCnt),(void *)pBuffer)!=SD_NO_ERR)
        {
          return FS_ERR;
        }
        /* Try to repair original FAT sector with contents of copy */
       FS__lb_write(FS__pDevInfo[0].harddisk_driver,0,(*pFATSector),(void *)pBuffer);
      }
      *pLastSector = *pFATSector;
    }
    if(FAT_type==FAT16){
      a = pBuffer[*pFATOffset];
      b = pBuffer[*pFATOffset + 1];
      fatentry = a | b;
    }
    if (fatentry != 0) {
      curclst++;  /* Cluster is in use or defect, so try the next one */
    }
  }
  if (fatentry == 0) {
    return curclst;  /* Free cluster found */
  }
  return FS_ERR;
}



/*********************************************************************
*
*             _FS__fat_SetEOFMark
*
  Description:
  FS internal function. Set the EOF mark in the FAT for a cluster.
  The function does not write the FAT sector. An exception is FAT12,
  if the FAT entry is in two sectors. 

  Parameters:
  FATSector   - FAT sector, where the cluster is located. 
  pLastSector - Pointer to an FS_i32, which contains the number of the 
                sector in pBuffer.
  FATOffset   - Offset of the cluster in the FAT sector.
  Cluster     - Cluster number, where to set the EOF mark.
  pBuffer     - Pointer to a sector buffer.
  FSysType    - ==1 => FAT12
                ==0 => FAT16
                ==2 => FAT32
  FATSize     - Size of one FAT ind sectors.
  BytesPerSec - Number of bytes in each sector.
 
  Return value:
  >=0         - EOF mark set.
  <0          - An error has occured.
*/

static S32 _FS__fat_SetEOFMark(U32 FATOffset, U8 *pBuffer,U16 FAT_type) {
  
  if(FAT_type==FAT16) { /* FAT16 */
    pBuffer[FATOffset]      = (U8)0xff;
    pBuffer[FATOffset + 1]  = (U8)0xff;
    return FS_NO_ERR;
  }
  return FS_ERR;
}



/*********************************************************************
*
*             _FS__fat_LinkCluster
*
  Description:
  FS internal function. Link the new cluster with the EOF mark to the 
  cluster list.

  Parameters:
  pLastSector - Pointer to an FS_i32, which contains the number of the 
                sector in pBuffer.
  Cluster     - Cluster number of the new cluster with the EOF mark.
  LastClust   - Number of cluster, to which the new allocated cluster
                is linked to.
  pBuffer     - Pointer to a sector buffer.
  FSysType    - ==1 => FAT12
                ==0 => FAT16
                ==2 => FAT32
  FATSize     - Size of one FAT ind sectors.
  BytesPerSec - Number of bytes in each sector.
 
  Return value:
  >=0         - Link has been made.
  <0          - An error has occured.
*/

static S32 _FS__fat_LinkCluster(S32 *pLastSector, U32 Cluster,
                                S32 LastClust, U8 *pBuffer, U8 FAT_type, 
                                U32 FATSize, U32 BytesPerSec) {
  U32 fatindex;
  U32 fatoffs;
  U32 fatsec;
  U8 a;
  U8 b;
  U8 err1;
  U8 err2;

  /* Link old last cluster to this one */
 
  if(FAT_type==FAT16){
    fatindex = LastClust * 2;               /* FAT16 */
  }
  fatsec =FS__pDiskInfo[0].FATStartSec + (fatindex / BytesPerSec);
  fatoffs = fatindex % BytesPerSec;
  if (fatsec != *pLastSector) {
    /* 
       FAT entry, which has to be modified is not in the same FAT sector, which is
       currently in the buffer. So write it to the media now.
    */
    err1 = FS__lb_write(FS__pDevInfo[0].harddisk_driver,0,(*pLastSector),(void *)pBuffer);
     
    err2 = FS__lb_write(FS__pDevInfo[0].harddisk_driver,0,((*pLastSector)+FS__pDiskInfo[0].FATSecCnt),(void *)pBuffer);
    if((err1!=SD_NO_ERR)||(err2!=SD_NO_ERR))
    {
    	  return FS_ERR;
    }
    err1 =FS__lb_read(FS__pDevInfo[0].harddisk_driver,0,fatsec,(void *)pBuffer);
    if (err1!=SD_NO_ERR) {
       err1 =FS__lb_read(FS__pDevInfo[0].harddisk_driver,0,(fatsec+FS__pDiskInfo[0].FATSecCnt),(void *)pBuffer);
      if (err1!=SD_NO_ERR) {
        return FS_ERR;
      }
      /* Try to repair original FAT sector with contents of copy */
     FS__lb_write(FS__pDevInfo[0].harddisk_driver,0,fatsec,(void *)pBuffer);
    }
    *pLastSector = fatsec;
  }
  //填入FAT表里的簇号
  a = Cluster & 0xff;
  b = (Cluster / 0x100L) & 0xff;
  if(FAT_type==FAT16) { /* FAT16 */
    pBuffer[fatoffs]      = a;
    pBuffer[fatoffs + 1]  = b;
    err1=FS__lb_write(FS__pDevInfo[0].harddisk_driver,0,fatsec,(void *)pBuffer);
    err2=FS__lb_write(FS__pDevInfo[0].harddisk_driver,0,(fatsec+FS__pDiskInfo[0].FATSecCnt),(void *)pBuffer);
    if((err1!=SD_NO_ERR)||(err2!=SD_NO_ERR))
    {
    	return FS_ERR;
    }
  }
  return FS_NO_ERR;
}

S32  FS__fat_checkunit(void)
{
     U16 err;
	 if(FS__pDiskInfo[0].Drive_unit==0)
	 {
	 	err=_FS_ReadBPB();
	 	if(err!=FS_NO_ERR)
	 	{
	 	    return err;
	 	}
	 	if(FS__pDiskInfo[0].NumFATs!=2)
	    {
	 	    return BPB_ERR;
	    }
	 }
 else if(FS__pDiskInfo[0].NumFATs!=2)
	  {
		 	err=_FS_ReadBPB();
		 	if(err!=FS_NO_ERR)
		 	{
		 	    return err;
		 	}
		 	if(FS__pDiskInfo[0].NumFATs!=2)
		    {
		 	    return BPB_ERR;
		    }
	  }
	 return FS_NO_ERR;
}


/*********************************************************************
*
*             FS__fat_FAT_find_eof
*
  Description:
  FS internal function. Find the next EOF mark in the FAT.

  Parameters:
  StrtClst    - Starting cluster in FAT.
  pClstCnt    - If not zero, this is a pointer to an FS_u32, which
                is used to return the number of clusters found
                between StrtClst and the next EOF mark.
 
  Return value:
  >=0         - Cluster, which contains the EOF mark.
  <0          - An error has occured.
*/

S32 FS__fat_FAT_find_eof( U32 StrtClst, U32 *pClstCnt) {
  U32 clstcount;
  U32 fatsize;
  U32 maxclst;
  U32 fatindex;
  U32 fatsec;
  U32 fatoffs;
  S32 lastsec;
  U32 curclst;
  U32 bytespersec;
  U32 eofclst;
  U8  fattype;
  U16 err;
  U8 *buffer;
  unsigned char a;
  unsigned char b;
  fattype = FS__fat_which_type();
  if(fattype==FAT16) {
    maxclst = 65525UL;      /* FAT16 */
  }
  buffer = FS__fat_malloc(FAT_SEC_SIZE);
  if ((S32)BUFFER_ERR==(S32)buffer) {
    return BUFFER_ERR;
  }
  fatsize = FS__pDiskInfo[0].FATSecCnt;
  bytespersec   = FS__pDiskInfo[0].BytsPerSec;
  curclst       = StrtClst;
  lastsec       = -1;
  clstcount     = 0;
  while (clstcount < maxclst) {
    eofclst = curclst;
    clstcount++;
    if(fattype==FAT16){
      fatindex = curclst * 2;               /* FAT16 */
    }
    fatsec  = FS__pDiskInfo[0].FATStartSec + (fatindex / bytespersec);
    fatoffs = fatindex % bytespersec;
    if (fatsec != lastsec) {
      err = FS__lb_read(FS__pDevInfo[0].harddisk_driver, 0, fatsec, (void*)buffer);
      if (err!=SD_NO_ERR) {
        err =FS__lb_read(FS__pDevInfo[0].harddisk_driver, 0, fatsec+fatsize, (void*)buffer);
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
      a         = buffer[fatoffs];
      b         = buffer[fatoffs + 1];
      curclst   = a + 256 * b;
      curclst  &= 0xffffL;
      if (curclst >= (S32)0xfff8L) {
        /* EOF found */
        FS__fat_free(buffer);
        if (pClstCnt) {
          *pClstCnt = clstcount;
        }
        return eofclst;
      }
    }
  } /* while (clstcount<maxclst) */
  FS__fat_free(buffer);
  return FS_ERR;
}

/*********************************************************************
*
*             FS__fat_FAT_alloc
*
  Description:
  FS internal function. Allocate a new cluster in the FAT and link it
  to LastClust. Assign an EOF mark to the new allocated cluster.
  The function has grown a lot, since it supports all FAT types (FAT12,
  FAT16 & FAT32). There is also room for performance improvement, when
  makeing the new FAT entry and the old entry is within the same FAT
  sector.

  Parameters:
  LastClust   - Number of cluster, to which the new allocated cluster
                is linked to. If this is negative, the new cluster is
                not linked to anything and only the EOF mark is set.
 
  Return value:
  >=0         - Number of new allocated cluster, which contains the 
                EOF mark.
  <0          - An error has occured.
*/

S32 FS__fat_FAT_alloc( S32 LastClust) {
  U32 fatsize;
  U32 fatoffs;
  U32 bytespersec;
  S32 curclst;
  U32 fatsec;
  S32 lastsec;
  U8 *buffer;
  U16 fattype;
  S32 err;
  S32 err2;
  S32 lexp;
  
  buffer = (U8*)FS__fat_malloc(FAT_SEC_SIZE);
  if ((S32)buffer==(S32)BUFFER_ERR) {
    return BUFFER_ERR;
  }
  fattype = FS__fat_which_type();
  fatsize = FS__pDiskInfo[0].FATSecCnt;
  bytespersec   = FS__pDiskInfo[0].BytsPerSec;
  /* Find a free cluster in the FAT */
  curclst       = _FS__fat_FindFreeCluster( &fatsec, &lastsec, &fatoffs, LastClust, buffer, fattype, fatsize, bytespersec);
  if (curclst <0) {
    FS__fat_free(buffer);   /* No free cluster found. */
    return FS_ERR;
  }
  /* Make an EOF entry for the new cluster */
  err = _FS__fat_SetEOFMark(fatoffs, buffer,fattype);
  if (err < 0) {
    FS__fat_free(buffer);
    return FS_ERR;
  }
  /* Link the new cluster to the cluster list */
  if (LastClust < 0) {
    err  = FS__lb_write(FS__pDevInfo[0].harddisk_driver, 0, lastsec, (void*)buffer);
    err2 = FS__lb_write(FS__pDevInfo[0].harddisk_driver, 0, lastsec+fatsize, (void*)buffer);
    lexp = (err < 0);
    lexp = lexp || (err2 < 0);
    if (lexp) {
      FS__fat_free(buffer);
      return FS_ERR;
    }
  }
  else {
    err = _FS__fat_LinkCluster(&lastsec, curclst, LastClust, buffer, fattype, fatsize, bytespersec);
    if (err < 0) {
      FS__fat_free(buffer);
      return FS_ERR;
    }
  }
  FS__fat_free(buffer);
  return curclst;
}

/*********************************************************************
*
*             FS__fat_diskclust
*
  Description:
  FS internal function. Walk through the FAT starting at StrtClst for
  ClstNum times. Return the found cluster number of the media. This is
  very similar to FS__fat_FAT_find_eof.
  
  Parameters:
  Idx         - Index of device in the device information table 
                referred by FS__pDevInfo.
  Unit        - Unit number.
  StrtClst    - Starting point for FAT walk.
  ClstNum     - Number of steps.
 
  Return value:
  > 0         - Number of cluster found after ClstNum steps.
  ==0         - An error has occured.
*/
//返回ClstNum步后寻找到的簇号，若ClstNum步前出现EOF返回错误
U32 FS__fat_diskclust( S32 StrtClst, S32 ClstNum) {
  U32 fatsize;
  S32 fatindex;
  S32 fatsec;
  S32 fatoffs;
  S32 lastsec;
  S32 curclst;
  S32 todo;
  S32 bytespersec;
  S32 err;
  U8 fattype;
  U8 *buffer;
  unsigned char a;
  unsigned char b;
  fattype = FS__fat_which_type();
  buffer = FS__fat_malloc(FAT_SEC_SIZE);
  if (BUFFER_ERR==(S32)buffer) {
    return 0;
  }
  fatsize = FS__pDiskInfo[0].NumFATs;
  bytespersec = FS__pDiskInfo[0].BytsPerSec;
  todo        = ClstNum;
  curclst     = StrtClst;
  lastsec     = -1;
  while (todo) {
    if(fattype==FAT16){
      fatindex = curclst * 2;               /* FAT16 */
    }
    fatsec  = FS__pDiskInfo[0].FATStartSec + (fatindex / bytespersec);
    fatoffs = fatindex % bytespersec;
    if (fatsec != lastsec) {
      err =FS__lb_read(FS__pDevInfo[0].harddisk_driver, 0, fatsec, (void*)buffer);
      if (err!=SD_NO_ERR) {
        err = FS__lb_read(FS__pDevInfo[0].harddisk_driver, 0, fatsize+fatsec, (void*)buffer);
        if (err!=SD_NO_ERR) {
          FS__fat_free(buffer);
          return 0;
        }
        /* Try to repair original FAT sector with contents of copy */
         FS__lb_write(FS__pDevInfo[0].harddisk_driver, 0, fatsec, (void*)buffer);
      }
      lastsec = fatsec;
    }
    if(fattype==FAT16){
      a = buffer[fatoffs];
      b = buffer[fatoffs + 1];
      curclst  = a + 256 * b;
      curclst &= 0xffffL;
      if (curclst >= (S32)0xfff8L) {
        FS__fat_free(buffer);
        return 0;
      }
    }
    todo--;
  }
  FS__fat_free(buffer);
  return curclst;
}



/*********************************************************************
*
*             Global Variables
*
**********************************************************************
*/

const FS__fsl_type FS__fat_functable = {
  "FAT16",
  FS__fat_fopen,        
  FS__fat_fclose,      
  FS__fat_fread,       
  FS__fat_fwrite,      
  0,                  
  0,                  
  0,      
};



