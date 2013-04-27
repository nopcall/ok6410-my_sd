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
       U8  Drive_unit;      //�߼���������
       U8  FATType;         //FAT����
       U8  SecPerClus;      //ÿ��������
       U8  NumFATs;         //FAT����Ŀ
       U32 StartSec;        //��������������ʼ����
       U32 SecPerDisk;      //�߼�����������������
       U32 BytsPerSec;      //ÿ�����ֽ���
       U32 RootDirTable;    //��Ŀ¼��ʼ������
       U32 RootSecCnt;      //��Ŀ¼ռ��������
       U32 FATStartSec;     //FAT��ʼ������
       U32 FATSecCnt;       //ÿ��FATռ��������
       U32 DataStartSec;    //��������ʼ������
       U32 ClusPerData;     //��������������
       U32 PathClusIndex;   //��ǰ·����FDT��ʼ�غţ�0λ��Ŀ¼     
       void *RsvdForLow;    //�������ײ���������
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
 