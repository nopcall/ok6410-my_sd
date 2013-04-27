#include "fs_int.h"
#include "fs_api.h"
#include "fs_clib.h"
#include "def.h"
#include "fs_fat.h"


/*********************************************************
*
*
*
*           Cache management funtions
*
*
**********************************************************
*/

/********************************************************
*
*                  FS__Cacheinit;
*      init all Caches,this should be used when start
*                  the file systerm
*
***********************************************************
*/
void FS__Cacheinit(void){
   U32 i;
   for(i=0;i<FS__pDevInfo[0].maxcache;i++){
     (FS__pDevInfo[0].pDevCacheInfo)[i].handle=0;
     (FS__pDevInfo[0].pDevCacheInfo)[i].fileOldOffset=-1;
     (FS__pDevInfo[0].pDevCacheInfo)[i].CacheOffset=0;
   }
}

/********************************************************
*
*                FS__FlusCache;
*           Clear the corresponding Cache
*
*
*********************************************************
*/

U32 FS__FlusCache(FS_FILE *handle){
   U32 i,tempOffset;
   tempOffset=handle->Offset;
   handle->Offset=(FS__pDevInfo[0].pDevCacheInfo)[handle->CacheId].fileOldOffset;
   i=FS_FWrite((FS__pDevInfo[0].pDevCacheInfo)[handle->CacheId].SecsDatabuff,1,
                            (FS__pDevInfo[0].pDevCacheInfo)[handle->CacheId].CacheOffset,handle);
   handle->Offset=tempOffset;
   handle->dirty=0;
   (FS__pDevInfo[0].pDevCacheInfo)[handle->CacheId].fileOldOffset=handle->Offset;
   (FS__pDevInfo[0].pDevCacheInfo)[handle->CacheId].CacheOffset=0;
   return i;
} 
/********************************************************
*
*               Cache_Malloc
*          try to Malloc a cache for a file
*          return -0 false
*                 -1 success
***********************************************************
*/
U8 Cache_Malloc(FS_FILE *pFile){
   S32 j,m,k;
   FS_FILE *handle;
   //存在空CACHE
   if(!pFile)return 0;
   if(pFile->CacheId>=0)return 1;
   if(FS__pDevInfo[0].freecache>0){
     	for(j=0;j<FS__pDevInfo[0].maxcache;j++){
     	  if((FS__pDevInfo[0].pDevCacheInfo)[j].handle==0){
     			break;
     	  }
     	}
     	(FS__pDevInfo[0].pDevCacheInfo)[j].handle=pFile;
     	pFile->CacheId=j;
     	(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset=pFile->Offset;
     	(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset=0;
     	pFile->dirty=0;
     	FS__pDevInfo[0].freecache--;
     	return 1;
   }
   //查找CACHE对应文件中非脏文件或空的CACHE
   for(j=0;j<FS__pDevInfo[0].maxcache;j++){
      handle=(FS__pDevInfo[0].pDevCacheInfo)[j].handle;
      if(handle->dirty==0||(FS__pDevInfo[0].pDevCacheInfo)[j].CacheOffset==0){
        pFile->CacheId=j;
        handle->CacheId=-1;
        handle->dirty=0;
        (FS__pDevInfo[0].pDevCacheInfo)[j].handle=pFile;
        (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset=pFile->Offset;
     	(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset=0;
     	pFile->dirty=0;
     	return 1;
      }
   }
   //查找CACHE中存放元素最多的作为替换
   m=(FS__pDevInfo[0].pDevCacheInfo)[0].CacheOffset;
   k=0;
   for(j=0;j<FS__pDevInfo[0].maxcache;j++){
      if((FS__pDevInfo[0].pDevCacheInfo)[j].CacheOffset>m){
          m=(FS__pDevInfo[0].pDevCacheInfo)[j].CacheOffset;
          k=j;
      }
   } 
   FS__FlusCache((FS__pDevInfo[0].pDevCacheInfo)[k].handle);
   pFile->CacheId=k;
   (FS__pDevInfo[0].pDevCacheInfo)[k].handle->CacheId=-1;
   (FS__pDevInfo[0].pDevCacheInfo)[k].handle=pFile;
   (FS__pDevInfo[0].pDevCacheInfo)[k].fileOldOffset=pFile->Offset;
   (FS__pDevInfo[0].pDevCacheInfo)[k].CacheOffset=0;
   pFile->dirty=0;
   return 1;
}
/********************************************************
*
*               Cache_Free
*          try to Free a corresponding Cache
*          return -0 false
*                 -1 success
***********************************************************
*/
U8  Cache_Free(FS_FILE*pFile){
    if(!pFile)return 0;
    if(pFile->CacheId<0)return 1;
    if(pFile->dirty==1&&(FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset>0){
       FS__FlusCache(pFile);
    }
    (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].fileOldOffset=-1;
    (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].CacheOffset=0;
    (FS__pDevInfo[0].pDevCacheInfo)[pFile->CacheId].handle=0;
    pFile->CacheId=-1;
    FS__pDevInfo[0].freecache++;
    return 1;
}