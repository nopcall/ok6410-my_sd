#ifndef __SD_H__
#define __SD_H__
#include "fs_int.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INICLK	400000//300000
#define NORCLK	5000000
#define SD_BLOCKSIZE  0x200




#define SDICCON_START (1<<8)
#define SDICCON_WRSP (1<<9)
#define SDICCON_LRSP (1<<10)

#define SDICSTA_SENT (1<<11)
#define SDICSTA_TOUT (1<<10)
#define SDICSTA_RSP (1<<9)

#define SDIDSTA_TOUT (1<<5)
#define SDIDSTA_DFIN (1<<4)
#define SDIDSTA_BFIN (1<<3)

#define SDIFSTA_RX (1<<12)
#define SDIFSTA_CNT 0x7F

#define SDIFSTA_TX (1<<13)

#define SDIIMSK_TOUT (1<<15)
#define SDIIMSK_DFIN (1<<7)
#define SDIIMSK_RX_LAST (1<<2)
#define SDIIMSK_RX_FULL (1<<1)
#define SDIIMSK_TX_EMP (1<<3)

#define SDIDCON_TARSP_1 (1<<20)
#define SDIDCON_RACMD_1 (1<<19)
#define SDIDCON_BLK (1<<17)
#define SDIDCON_WIDE (1<<16)
#define SDIDCON_DMA (1<<15)
#define SDIDCON_RX (0x02<<12)
#define SDIDCON_TX  (0x03<<12)


#define SDICON_FRESET (1<<1)

#define SDICON_LE (1<<4)
#define SDICON_ENCLK (1<<0)

#define R0   0
#define R1   1
#define R1B  2
#define R2   3
#define R3   4
#define R6   5

//#define CMD(x)   ((x&0x3f)|0x40)
#define CMD0     0x40
#define CMD2     0x42
#define CMD3     0x43
#define CMD7     0x47
#define CMD9     0x49
#define CMD10    0x4a
#define CMD11    0x4b
#define CMD12    0x4c
#define CMD13    0x4d
#define CMD15    0x4f
#define CMD16    0x50
#define CMD17    0x51
#define CMD18    0x52
#define CMD24    0x58
#define CMD25    0x59
#define CMD27    0x5b
#define CMD28    0x5c
#define CMD29    0x5d
#define CMD30    0x5e
#define CMD32    0x60
#define CMD33    0x61
#define CMD38    0x66
#define CMD55    0x77
#define CMD56    0x78
#define ACMD6    0x46
#define ACMD13   0x4d
#define ACMD22   0x56
#define ACMD23   0x57
#define ACMD41   0x69
#define ACMD42   0x6a
#define ACMD51   0x73


#define CMD0_R     R0
#define CMD2_R     R2
#define CMD3_R     R6
#define CMD7_R     R1
#define CMD9_R     R2
#define CMD10_R    R2
#define CMD11_R    R1
#define CMD12_R    R1B
#define CMD13_R    R1
#define CMD15_R    R0
#define CMD16_R    R1
#define CMD17_R    R1
#define CMD18_R    R1
#define CMD24_R    R1
#define CMD25_R    R1
#define CMD27_R    R1
#define CMD28_R    R1B
#define CMD29_R    R1B
#define CMD30_R    R1
#define CMD32_R    R1
#define CMD33_R    R1
#define CMD38_R    R1B
#define CMD55_R    R1
#define CMD56_R    R1
#define ACMD6_R    R1
#define ACMD13_R   R1
#define ACMD22_R   R1
#define ACMD23_R   R1
#define ACMD41_R   R3
#define ACMD42_R   R1
#define ACMD51_R   R1



#define MAX_CARD          1
#define CMD_ERR           0
#define SD_CARD           1
#define MMC_CARD          2
#define SD_NO_ERR         3
#define SD_ERR            4
#define NO_CARD           5


U8   SD_SendCommand(U8 cmd, U32 param, U8 resptype, U8 *resp);
U8   SD_SelectCard(U16 RCA);
U8   SD_DeSelectCard(void);
U8   SD_Initialize(SDFSDisk *sds);



extern volatile unsigned int TR_end;

void __irq DMA_end(void);
int  Chk_DATend(void);
int  Chk_BUSYend(void);
U8   SD_ReadCard_Status(U16 RCA,U8 len,U8 status[]);
U8   SD_ReadBlock(U32 blocknum,U8 *recbuf);
U8   SD_WriteBlock(U32 blocknum,U8 *recbuf);
U8   SD_EraseSector(const SDFSDisk *sds,U32 sectornum);


// Function prototypes
void Test_SDI(void);
void TR_Buf_new(void);
U8 Rd_Block(U32 block,U32 src,U32 *dst);
U8 Wt_Block(U32 block,U32 *src,U32 dst);
void Flush_Rx_buf(void);
U8 Set_4bit_bus(U16 RCA);
void View_Rx_buf(void);
void View_Tx_buf(void);


#ifdef __cplusplus
}
#endif
#endif /*__SD_H___*/
