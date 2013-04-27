#ifndef _FS_FAT_H_
#define _FS_FAT_H_
#include  "def.h"
#include  "fs_api.h"



#define     FAT_SEC_SIZE           0x200
#define     MAX_DRIVERS                1

#define     FAT12                   0x12
#define     FAT16                   0x16
#define     FAT32                   0x32

#define     FS_FAT_ATTR_READ_ONLY   0x01
#define     FS_FAT_ATTR_HIDDEN      0x02
#define     FS_FAT_ATTR_SYSTEM      0x04
#define     FS_FAT_VOLUME_ID        0x08
#define     FS_FAT_ATTR_ARCHIVE     0x20
#define     FS_FAT_ATTR_DIRECTORY   0x10

#define     FS_FAT_DENTRY_SIZE      0x20



#define     BUFFER_ERR      -3
#define     FS_NO_ERR        1
#define     FS_ERR          -1
#define     BPB_ERR         -4
#define     DIR_FULL        -2

typedef struct _Disk_Info
{
       U8  Drive_unit;      //逻辑驱动器号
       U8  FATType;         //FAT类型
       U8  SecPerClus;      //每簇扇区数
       U8  NumFATs;         //FAT表数目
       U32 StartSec;        //整个分区物理起始扇区
       U32 SecPerDisk;      //逻辑驱动器包含扇区数
       U32 BytsPerSec;      //每扇区字节数
       U32 RootDirTable;    //根目录开始扇区号
       U32 RootSecCnt;      //根目录占用扇区数
       U32 FATStartSec;     //FAT开始扇区数
       U32 FATSecCnt;       //每个FAT占用扇区数
       U32 DataStartSec;    //数据区开始扇区号
       U32 ClusPerData;     //数据区包含簇数
       U32 PathClusIndex;   //当前路径的FDT表开始簇号，0位根目录     
       void *RsvdForLow;    //保留给底层驱动程序
}Disk_Info;




/* FAT directory entry */
typedef struct {
  U8   data[32];
} FS__fat_dentry_type;

 
 extern   Disk_Info    *const FS__pDiskInfo;
 
 
 /*********************************************************************
*
*             fat_misc
*/

void                FS__fat_block_init(void);
U8                  *FS__fat_malloc(U32 Size);
void                FS__fat_free(void *pBuffer);
S32                 FS__fat_FAT_alloc( S32 LastClust);
S32                 FS__fat_FAT_find_eof( U32 StrtClst, U32 *pClstCnt);
S32                 FS__fat_checkunit(void);
U32                 FS__fat_diskclust( S32 StrtClst, S32 ClstNum);
U8                  FS__fat_which_type(void);

/***********************************************************************
*
*            fat_open
*/
void FS__fat_make_realname(S8 *pEntryName, const S8 *pOrgName);
U32 FS__fat_dir_size( U32 DirStart);
U32 FS__fat_dir_realsec( U32 DirStart, U32 DirSec);
U32 FS__fat_find_dir( S8 *pDirName, U32 DirStart,U32 DirSize);
U32 FS__fat_findpath( const S8 *pFullName, S8 **pFileName,U32 *pDirStart);
FS_FILE *FS__fat_fopen(S8 *pFileName, const S8 *pMode, FS_FILE *pFile);



/***********************************************************************
*
*            fat_in
*/

U32 FS__fat_fread(void *pData, U32 Size, U32 N, FS_FILE *pFile);


/***********************************************************************
*
*            fat_out
*/

S32 FS__fat_fwrite(const void *pData, U32 Size, U32 N, FS_FILE *pFile);
S32 FS__fat_fclose(FS_FILE *pFile);
#endif
 