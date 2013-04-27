#include "fs_int.h"
#include "fs_api.h"
#include "fs_clib.h"
#include "def.h"
#include "fs_fat.h"


/*********************************************************************
*
*             Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*             FS_FRead
*
  Description:
  API function. Read data from a file.

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

U32 FS_FRead(void *pData, U32 Size, U32 N, FS_FILE *pFile) {
  U32 i;

  if (!pFile) {
    return 0;  /* No pointer to a FS_FILE structure */
  }
  if (!pFile->mode_r) {
    /* File open mode does not allow read ops */
    return 0;
  }
  
  i = 0;
  /*
  if (pFile->CacheId<0){
     if(FS__pDevInfo[0].freecache>0){
     	for(j=0;j<FS__pDevInfo[0].maxcache;j++){
     	  if((FS__pDevInfo[0].pDevCacheInfo)[j].file_id<0){
     			break;
     	  }
     	}
     	for(k=0;k<FS_MAXOPEN;k++){
     	  if(pFile==FS_Get_Fhandle(k))
     	    break;
     	}
     	(FS__pDevInfo[0].pDevCacheInfo)[j].file_id=k;
     	pFile->CacheId=j;
     	FS__pDevInfo[0].freecache--;
     	i = (FS__pDevInfo[0].fs_ptr->fsl_fread)(pData, Size, N, pFile);
     	if(i>CACHE_BUFFER_SIZE/2){
     	  m=CACHE_BUFFER_SIZE/2;
     	}
     	else
     	  m=i;
     	for(j=1;j<=m;j++){
     	  (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].SecsDatabuff[m-j]= (*((S8*)(((S8*)pData) +i-j)));
     	}
     	(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset=pFile->Offset-m;
     	(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset=m;
        return i;
     }
     else{
       i = (FS__pDevInfo[0].fs_ptr->fsl_fread)(pData, Size, N, pFile);
       return i;
     }
  }
  else{
     //全部不在CACHE中
    if(pFile->Offset>=(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset
                                        +(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset||
                                        pFile->Offset+N*Size<=(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset||
                                                                        pFile->Offset<(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset){
       if(pFile->dirty==1&&(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset>0){
         FS__FlusCache(pFile);
         (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset=0;
         pFile->dirty=0;
       }
       i = (FS__pDevInfo[0].fs_ptr->fsl_fread)(pData, Size, N, pFile);
       if(i>CACHE_BUFFER_SIZE/2){
     	  m=CACHE_BUFFER_SIZE/2;
       }
       else
     	  m=i;
       for(j=1;j<=m;j++){
     	 (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].SecsDatabuff[m-j]= (*((S8*)(((S8*)pData) +i-j)));
       }
       (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset=pFile->Offset-m;
       (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset=m;  
       return i;                  
    }
    //全部存在于CACHE中
    if(pFile->Offset>=(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset&&
                          pFile->Offset+N*Size<=(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset+
                                                    (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset){
      for(i=0;i<N*Size;i++){
        *((S8*)(((S8*)pData) +i))=(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].SecsDatabuff[pFile->Offset-(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset+i];
      }
      return (i+1);
    }
    //部分存在于CACHE中，有三种情况下面是第一种
    if(pFile->Offset>=(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset&&
                         pFile->Offset+N*Size>(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset+
                                                    (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset){
      for(i=0;i<(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset+
                             (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset-pFile->Offset;i++){
        *((S8*)(((S8*)pData) +i))=(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].SecsDatabuff[pFile->Offset-(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset+i];
      }
      pFile->Offset+=i;
      i+=(FS__pDevInfo[0].fs_ptr->fsl_fread)((S8*)(((S8*)pData)+i), 1, N*Size-i, pFile);
      if(pFile->Offset-(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset>=CACHE_BUFFER_SIZE){
         if(pFile->dirty==1&&(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset>0){
         FS__FlusCache(pFile);
         (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset=0;
         pFile->dirty=0;
       }
       m=CACHE_BUFFER_SIZE/2;
       for(j=1;j<=m;j++){
     	 (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].SecsDatabuff[m-j]= (*((S8*)(((S8*)pData) +i-j)));
       }
       (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset=pFile->Offset-m;
       (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset=m;  
      }
      else{
        for(;(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset<pFile->Offset-(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset;
                                                                                                   (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset++){
          (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].SecsDatabuff[(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset]
                                        =(*((S8*)(((S8*)pData) +pFile->Offset-(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset-
                                                                                       (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset)));
        }
      }
      return i;
    }
    //部分存在于CACHE中，下面是第二种情况 
  }
  */
  if (FS__pDevInfo[0].fs_ptr->fsl_fread) {
    i = (FS__pDevInfo[0].fs_ptr->fsl_fread)(pData, Size, N, pFile);
  }
  
  return i; 
}
