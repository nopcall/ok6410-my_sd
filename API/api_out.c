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
*             FS_FWrite
*
  Description:
  API function. Write data to a file.

  Parameters:
  pData       - Pointer to a data to be written to a file. 
  Size        - Size of an element to be transferred.
  N           - Number of elements to be transferred to the file.
  pFile       - Pointer to a FS_FILE data structure.
  
  Return value:
  Number of elements written.
*/

S32 FS_FWrite(const void *pData, U32 Size, U32 N, FS_FILE *pFile) {
  U32 i;
  if (!pFile) {
    return 0; /* No pointer to a FS_FILE structure */
  }
  if (!pFile->mode_w) {
    /* Open mode does now allow write access */
    return 0;
  }
  i = 0;
  /*
  if(pFile->CacheId<0){
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
  	if(N*Size>=CACHE_BUFFER_SIZE){
  	  i = (FS__pDevInfo[0].fs_ptr->fsl_fwrite)(pData, Size, N, pFile);
  	  (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset=pFile->Offset;
  	  (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset=0;
  	  pFile->dirty=0;
  	  return i;
  	}
  	else{
  	  (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset=pFile->Offset;
  	  for(i=0;i<N*Size;i++){
  	    (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].SecsDatabuff[i]= (*((S8*)(((S8*)pData) + i)));
  	  }
  	  (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset=i+1;
  	  pFile->dirty=1;
  	  return (i+1);
  	}
   }
   else{
     i = (FS__pDevInfo[0].fs_ptr->fsl_fwrite)(pData, Size, N, pFile);
     return i;
   }
  }
  else{
    if(pFile->Offset<=(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset&&
                      pFile->Offset+N*Size>=(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset+
                                                   (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset){
       if(N*Size>=CACHE_BUFFER_SIZE){
         i = (FS__pDevInfo[0].fs_ptr->fsl_fwrite)(pData, Size, N, pFile);
         (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset=pFile->Offset;
         (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset=0;
         pFile->dirty=0;
         return i;
       } 
       else{
         (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset=pFile->Offset;
  	     for(i=0;i<N*Size;i++){
  	       (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].SecsDatabuff[i]= (*((S8*)(((S8*)pData) + i)));
  	     }
  	     (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset=i+1;
  	     pFile->dirty=1;
  	     return (i+1);
      }                                             
    }
    if((pFile->Offset<(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset&&
                        pFile->Offset+N*Size<(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset+
                                                   (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset)||
                                           (pFile->Offset>(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset+
                                                                (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset)){
       if(pFile->dirty==1&&(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset>0)FS__FlusCache(pFile);
       if(N*Size>=CACHE_BUFFER_SIZE){
         i = (FS__pDevInfo[0].fs_ptr->fsl_fwrite)(pData, Size, N, pFile);
         (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset=pFile->Offset;
         (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset=0;
         pFile->dirty=0;
         return i;
       } 
       else{
         (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset=pFile->Offset;
  	     for(i=0;i<N*Size;i++){
  	       (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].SecsDatabuff[i]= (*((S8*)(((S8*)pData) + i)));
  	     }
  	     (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset=i+1;
  	     pFile->dirty=1;
  	     return (i+1);
      }                                                                 
    }
  }
  */
  if (FS__pDevInfo[0].fs_ptr->fsl_fwrite) {
    i = (FS__pDevInfo[0].fs_ptr->fsl_fwrite)(pData, Size, N, pFile);
  }
  return i;  
}
