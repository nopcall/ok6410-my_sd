#ifndef __FS_API_H__
#define __FS_API_H__



/* Global error codes */
#define FS_ERR_OK                 (S16)0x0000
#define FS_ERR_EOF                (S16)0xfff0
#define FS_ERR_DISKFULL           (S16)0xffe0
#define FS_ERR_INVALIDPAR         (S16)0xffd0
#define FS_ERR_WRITEONLY          (S16)0xffc0
#define FS_ERR_READONLY           (S16)0xffb0
#define FS_ERR_READERROR          (S16)0xffa0
#define FS_ERR_WRITEERROR         (S16)0xff90
#define FS_ERR_DISKCHANGED        (S16)0xff80
#define FS_ERR_CLOSE              (S16)0xff70

/* Global constants*/
#define FS_SEEK_CUR         1
#define FS_SEEK_END         2
#define FS_SEEK_SET         0

/* I/O commands */
#define FS_CMD_FLUSH_CACHE        1000L
#define FS_CMD_CHK_DSKCHANGE      1010L
#define FS_CMD_READ_SECTOR        1100L
#define FS_CMD_WRITE_SECTOR       1110L
#define FS_CMD_FORMAT_MEDIA       2222L
#define FS_CMD_FORMAT_AUTO        2333L
#define FS_CMD_INC_BUSYCNT        3001L
#define FS_CMD_DEC_BUSYCNT        3002L
#define FS_CMD_GET_DISKFREE       4000L
#define FS_CMD_GET_DEVINFO        4011L
#define FS_CMD_FLASH_ERASE_CHIP   9001L


#define     FS_MAXOPEN              0x20
#define     FS_DIR_MAXOPEN          0x20

typedef struct _FILE{
  S8   Name[11];
  U8   Drive;
  U32  DirClus;
  U32  FileSize;
  U32  FstClus;
  U32  CurClust;
  S32  EOFClust;
  S32  CacheId;
  U32  Offset;
  S16  error;
  U8   dirty;
  U8   inuse;
  U8   mode_r;
  U8   mode_w;
  U8   mode_a;
  U8   mode_c;
  U8   mode_b;
}FS_FILE;  

typedef struct _FDT{
     char  Name[11];       //短文件名主文件名
     U8    Attr;           //文件属性
     U8	   NTRes;          //保留给NT
     U8    CrtTimeTenth;   //建立时间（FAT16保留）
     U16   CrtTime;        //建立时间（FAT16保留）
     U16   CrtDate;        //建立时日期（FAT16保留）
     U16   LstAccDate;     //最后访问日期
     U16   FstClusHI;      //起始簇号高两个字节
     U16   WrtTime;        //最后写时间
     U16   WrtDate;        //最后写日期
     U16   FstClusLO;      //起始簇号低两个字节
     U32   FileSize;       //文件大小
 }FS_DIR;
 
 
 
 
/*********************************************************************
*
*             Global function prototypes
*
**********************************************************************
*/

/*********************************************************************
*
*             STD file I/O functions
*/

FS_FILE             *FS_FOpen(const S8 *pFileName, const S8 *pMode);
void                FS_FClose(FS_FILE *pFile);
U32                 FS_FRead(void *pData, U32 Size, U32 N, FS_FILE *pFile);
S32                 FS_FWrite(const void *pData, U32 Size, U32 N, FS_FILE *pFile);


/*********************************************************************
*
*             file pointer handling
*/

S32   FS_FSeek(FS_FILE *pFile, U32 Offset, S32 Whence);
S32   FS_FTell(FS_FILE *pFile);


/*********************************************************************
*
*             I/O error handling
*/

S16                FS_FError(FS_FILE *pFile);
void                FS_ClearErr(FS_FILE *pFile);


/*********************************************************************
*
*             file functions
*/

S32 FS_Remove(const S8 *pFileName);


/*********************************************************************
*
*             IOCTL
*/

S32                 FS_IoCtl(const S8 *pDevName, S32 Cmd, S32 Aux, void *pBuffer);



/*********************************************************************
*
*             file system control functions
*/

void                 FS_Init(void);
void                 FS_Exit(void);
FS_FILE *            FS_Get_Fhandle(S32 file_id);


/********************************************************************
*
*                 FS__Cache funtions
*
*/
U32    FS__FlusCache(FS_FILE *handle);
void   FS__Cacheinit(void);
 
 #endif