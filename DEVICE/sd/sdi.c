#include <stdio.h>
#include <string.h>
#include "def.h"
#include "option.h"
#include "2410addr.h"
#include "2410lib.h"
#include "sdi.h"
#include "fs_int.h"

SDFSDisk SanDisk_info_table={
         0,
         N_CARD,
         0,
         0,
         0,
         0,
         0,
         0,
         
 };
/***********************************/
/*********glocbal data**************/
/************************************/

const SDDriver  SanDisk_driver_table={

		 SD_ReadBlock,
		 SD_WriteBlock,
		 0,
		 0,
		 
};
// Global variables
U8  *Tx_buffer;	//128[word]*16[blk]=8192[byte]
U8  *Rx_buffer;	//128[word]*16[blk]=8192[byte]
volatile unsigned int TR_end=0;
int Wide=1;
static U32 card_id=0;

/*****************************************
  SD卡测试函数
  函数名: Test_SDI
  描述: SD卡测试函数
  返回值：void
 *****************************************/
void Test_SDI(void)
{
   
    Uart_Printf("\n[SDI test]\n");
    
    SD_Initialize(FS__pDevInfo[0].harddisk_info);
    Uart_Printf("block_num=0x%x\n",FS__pDevInfo[0].harddisk_info->block_num);
    Uart_Printf("block_len=0x%x\n",FS__pDevInfo[0].harddisk_info->block_len);
    Uart_Printf("sector_size=0x%x\n",FS__pDevInfo[0].harddisk_info->sector_size);
    Uart_Printf("card_type=0x%x\n",FS__pDevInfo[0].harddisk_info->card_type);
    
    TR_Buf_new();
/*
INOM:    
    Uart_Printf("How many blocks to test?(1~16)?");
    block=(U32)Uart_GetIntNum();
    if((block==0)|block>16)
	goto INOM;
    //block=1;//tark
    Uart_Printf("\n");
*/

    //CMD13();
    SD_WriteBlock(&SanDisk_info_table,(U32)0,(U8 *)Tx_buffer);
    SD_ReadBlock(&SanDisk_info_table,(U32)0,(U8 *)Rx_buffer);
    View_Rx_buf();
    // Card deselect

    //SDIDCON  BlkNum                           [11: 0] = 0 : Block Number (0~4095).
    //SDIDCON  Data Transfer Mode (DatMode)     [13:12] = 0 : ready,
    //SDIDCON  Stop by force (STOP)             [14   ] = 0 : normal
    //SDIDCON  DMA Enable(EnDMA)			    [15   ] = 0 : disable(polling),
    //SDIDCON  Wide bus enable (WideBus)        [16   ] = 0 : standard bus mode(only SDIDAT[0] used),
    //SDIDCON  Block mode (BlkMode)  		     [17   ] = 0 : stream data transfer, 
    //SDIDCON  Busy AfterCommand(BACMD)         [18   ] = 0 : directly after DatMode set,
    //SDIDCON  Receive After Command (RACMD)    [19   ] = 0 : directly after DatMode set,
    //SDIDCON  Transmit After Response(TARSP)   [20  ] = 0 : directly after DatMode set,
    //SDIDCON  SDIO InterruptPeriodType(PrdType)[21   ] = 0 : exactly 2 cycle,
	rSDIDCON=0;
    //SDICSTA  RspIndex                     [7:0]  R    Response index 6bit with start 2bit (8bit)
    //SDICSTA  CMD line progress On (CmdOn) [8  ]  R    Command transfer in progress.
    //SDICSTA  Response Receive End (RspFin)[9  ] = 1 : response end
    //SDICSTA  Command Time Out (CmdTout)   [10 ] = 1 : timeout
    //SDICSTA  Command Sent (CmdSent)       [11 ] = 1 : command end
    //SDICSTA  Response CRC Fail(RspCrc     [12 ] = 1 : crc fail
    rSDICSTA=0xffff;
}

void TR_Buf_new(void)
{
    //-- Tx & Rx Buffer initialize
    int i, j;
    int start = 0x03020100;

    Tx_buffer=(U8 *)0x31000000;

    j=0;
    for(i=0;i<2048;i++)	//128[word]*16[blk]=8192[byte]
    {
       j+=0;
       *(Tx_buffer+j)=0xaa;
       j+=1;
       *(Tx_buffer+j)=0xbb;
       j+=2;
       *(Tx_buffer+j)=0xcc;
       j+=3;
	   *(Tx_buffer+j)=0xdd;   //i+j's value sent into the address
	   j+=4;
	}
//	*(Tx_buffer+i)=0x5555aaaa;
    Flush_Rx_buf();
/*
    for(i=0;i<20;i++){
        for(j=0;j<128;j++){
	Tx_buffer[j+i*128]=start;
	if(j % 64 == 63) start = 0x0302010;
	else start = start + 0x04040404;
        }
        start = 0x03020100;
    }
*/
}

void Flush_Rx_buf(void)
{
    //-- Flushing Rx buffer 
    int i;
    int j=0;

    Rx_buffer=(U8 *)0x31800000;
    for(i=0;i<2048;i++)	//128[word]*16[blk]=8192[byte]
    {
        j+=0;
	    *(Rx_buffer+j)=0;
	    j+=1;
	     *(Rx_buffer+j)=0;
	    j+=2;
	     *(Rx_buffer+j)=0;
	    j+=3;
	    *(Rx_buffer+j)=0;
	    j+=4;
	}
//    Uart_Printf("\n--End Rx buffer flush\n");
}
/*****************************************
  写入和读出缓冲区数据检查函数
  函数名: View_Rx_buf
  描述: 检测写入和输出数据
  返回值：
*****************************************/

void View_Rx_buf()
{
    //-- Display Rx buffer 
    int i,error=0;
/*
    for(i=0;i<2048;i++)
	Uart_Printf("RB[%02x]=%x,",Rx_buffer[i]);
*/
    Tx_buffer=(U8 *)0x31000000;
    Rx_buffer=(U8 *)0x31800000;
    Uart_Printf("Check Rx data\n");
    for(i=0;i<128;i++)
    {
        if(Rx_buffer[i] != Tx_buffer[i])
	    {
	    Uart_Printf("\nTx/Rx error\n"); 
	    Uart_Printf("%d:Tx-0x%08x, Rx-0x%08x\n",i,Tx_buffer[i], Rx_buffer[i]);
	    error=1;
	    break;
        }
        Uart_Printf(".");
    }
    if(!error)
	Uart_Printf(" O.K.\n");
}

void View_Tx_buf(void)
{
    //-- Display Tx buffer 
    int i;

    for(i=0;i<2048;i++)
	Uart_Printf("TB[%02x]=%x,",Tx_buffer[i]);
}

/*****************************************
  SD卡初始化函数
  函数名: SD_card_init
  描述: SD卡的初始化
  返回值：void
*****************************************/
U8 Card_sel_desel(U16 RCA,char sel_desel)
{
    int m,n;
    //-- Card select or deselect
    if(sel_desel)
    {
	    for(n=0;n<10;n++)
	    {
	       for(m=0;m<10;m++)
	       {
				//SDICARG  CmdArg  [31:0] = RCA<<16 :   CMD7(RCA,stuff bit)
				rSDICARG=RCA<<16;	
				
				//SDICCON CmdIndex              [7:0] = 0X47 : Command index with start 2bit (8bit)
			    //SDICCON Command Start(CMST)   [8  ] = 1    : command start
			    //SDICCON WaitRsp               [9  ] = 1    : wait response			    //SDICCON LongRsp               [10 ] = 0    : short response
			  	rSDICCON= (0x1<<9)|(0x1<<8)|0x47; // sht_resp, wait_resp, start, CMD7

				//-- Check end of CMD7
				if(SD_NO_ERR==Chk_CMDend(7, 1))break;
		    }
			//rSDICSTA=0xa00;	// Clear cmd_end(with rsp)

			//--State(transfer) check
			if( rSDIRSP0 & 0x1e00==0x800 )return SD_NO_ERR;
	    }
	    return SD_ERR;
    }
    else
    {
		for(n=0;n<100;n++)
		{
			//SDICARG  CmdArg  [31:0] = 0<<16 :   CMD7(RCA,stuff bit)
			rSDICARG=RCA<<16;		//CMD7(RCA,stuff bit)
			
			//SDICCON CmdIndex              [7:0] = 0X47 : Command index with start 2bit (8bit)
		    //SDICCON Command Start(CMST)   [8  ] = 1    : command start
		    //SDICCON WaitRsp               [9  ] = 0    : no respons
		    //SDICCON LongRsp               [10 ] = 0    : short response
		 	rSDICCON=(0x1<<8)|0x47;	//no_resp, start, CMD7

			//-- Check end of CMD7
			if(SD_NO_ERR==Chk_CMDend(7, 0))return SD_NO_ERR;
		 }
		 return SD_ERR;
		//rSDICSTA=0x800;	// Clear cmd_end(no rsp)
    }
}
void __irq DMA_end(void)
{
    ClearPending(BIT_DMA0);
    
    TR_end=1;
}
/*****************************************
  读存储块函数
  函数名: Rd_Block
  描述: 通过三种方式读存储块
  返回值：void
*****************************************/

U8 Rd_Block(U32 block,U32 *src,U32 *dst)
{    
    int m,n;

	//SDICON FIFO Reset (FRST) [1] = 1 : FIFO reset
    rSDICON |= rSDICON|(1<<1);	
    
	//SDICARG  CmdArg  [31:0] = 0 :  CMD17/18(addr)
    rSDICARG=(int)src;

//RERDCMD:
        for(n=0;n<100;n++)
        {
		    //use dma
		    pISR_DMA0=(unsigned)DMA_end;
		    //INTMSK INT_DMA0  [17] = 0 : Service availa
		    rINTMSK = ~(BIT_DMA0);

			//DISRC0  S_ADDR  [30:0] = SDIDAT : SDIDAT
		    rDISRC0=SDIDAT;	// SDIDAT
		    
		    //DISRCC0 INC [0] = 1 : Fixed
		    //DISRCC0 LOC [1] = 1 : the source is in the peripheral bus (APB).
		    rDISRCC0=(1<<1)+(1<<0);	
		    
		    rDIDST0=(int)dst;	// Rx_buffer
		    
		    //DIDSTC0 INC [0] = 0 : Increment
		    //DIDSTC0 LOC [1] = 0 : AHB
		    rDIDSTC0=(0<<1)+(0<<0);	// AHB, inc
		   
		    //DCON0 TC          [19: 0] = 10000000 : Initial transfer count (or transfer beat).
		    //DCON0 DSZ 		[21:20] = 10	   : Word
		    //DCON0 RELOAD      [22   ] = 1        : auto-reload of
		    //DCON0 SWHW_SEL    [23   ] = 1        : H/W request
		    //DCON0 HWSRCSEL    [26:24] = 10       : SDI
		    //DCON0 SERVMODE    [27   ] = 0        : single service
		    //DCON0 TSZ         [28   ] = 0        : single tx
		    //DCON0 INT         [29   ] = 1		   : TC int
		    //DCON0 SYNC        [30   ] = 0        : sync PCLK
		    //DCON0 DMD_HS      [31   ] = 1        : handshake
		    rDCON0=(1<<31)+(0<<30)+(1<<29)+(0<<28)+(0<<27)+(2<<24)+(1<<23)+(1<<22)+(2<<20)+128*block;
		    
		    //DMASKTRIG0  SW_TRIG [0] = 0 : no-sw trigger 
		    //DMASKTRIG0  ON_OFF  [1] = 1 : DMA2 channel on
		    //DMASKTRIG0  STOP    [2] = 0 : no-stop
		    rDMASKTRIG0=(0<<2)+(1<<1)+0;   

			//SDIDCON  BlkNum                           [11: 0] = block : Block Number (0~4095).
		    //SDIDCON  Data Transfer Mode (DatMode)     [13:12] = 2     : Rx start
		    //SDIDCON  Stop by force (STOP)             [14   ] = 0     : normal
		    //SDIDCON  DMA Enable(EnDMA)			    [15   ] = 1     : dma enable,
		    //SDIDCON  Wide bus enable (WideBus)        [16   ] = Wide  : 4bit bus
		    //SDIDCON  Block mode (BlkMode)  		    [17   ] = 1     : blk 
		    //SDIDCON  Busy AfterCommand(BACMD)         [18   ] = 0     : directly after DatMode set,
		    //SDIDCON  Receive After Command (RACMD)    [19   ] = 1     : Rx after cmd
		    //SDIDCON  Transmit After Response(TARSP)   [20   ] = 0     : directly after DatMode set,
		    //SDIDCON  SDIO InterruptPeriodType(PrdType)[21   ] = 0     : exactly 2 cycle,
			rSDIDCON=(1<<19)|(1<<17)|(Wide<<16)|(1<<15)|(2<<12)|(block<<0);
			    // Rx after rsp, blk, 4bit bus, dma enable, Rx start, blk num
		    if(block<2)	// SINGLE_READ
		    {
		   	//SDICCON CmdIndex              [7:0] = 0X51 : CMD17
		    //SDICCON Command Start(CMST)   [8  ] = 1    : command start
		    //SDICCON WaitRsp               [9  ] = 1    : wait_resp
		    //SDICCON LongRsp               [10 ] = 0    : short response
			rSDICCON=(0x1<<9)|(0x1<<8)|0x51;    // sht_resp, wait_resp, dat, start, CMD17
			if(SD_NO_ERR==Chk_CMDend(17, 1))	//-- Check end of CMD17
			    break;	    
		    }
		    else	// MULTI_READ
		    {
			//SDICCON CmdIndex              [7:0] = 0X52 : CMD18
		    //SDICCON Command Start(CMST)   [8  ] = 1    : command start
		    //SDICCON WaitRsp               [9  ] = 1    : wait_resp
		    //SDICCON LongRsp               [10 ] = 0    : short response
			rSDICCON=(0x1<<9)|(0x1<<8)|0x52;    // sht_resp, wait_resp, dat, start, CMD18
			if(SD_NO_ERR==Chk_CMDend(18, 1))	//-- Check end of CMD18 
			   break;
		    }
	    }
	    if(n==100)return SD_ERR;

	    //rSDICSTA=0xa00;	// Clear cmd_end(with rsp)
	    while(!TR_end);
		//Uart_Printf("rSDIFSTA=0x%x\n",rSDIFSTA);
	    
	    //INTMSK  BIT_DMA0 [17] = 1 : MASK
	    rINTMSK |= (BIT_DMA0);
		TR_end=0;

		//DMASKTRIG0  STOP [2] = 1 : DMA0 stop
		rDMASKTRIG0=(1<<2);	

    //-- Check end of DATA
    if(!Chk_DATend())
    { 
	    Uart_Printf("dat error\n");
	    rSDIDSTA=0x10;
	    return SD_ERR;
	}

	//SDIDSTA  Rx Data Progress On (RxDatOn) [0 ]  R  : Data receive in progress.
	//SDIDSTA  Tx Data progress On (TxDatOn) [1 ]  R  : Data transmit in progress.
	//SDIDSTA  Start Bit Error (SbitErr)     [2 ] =0  : not detect,
	//SDIDSTA  Busy Finish (BusyFin)         [3 ] =0  : not detect,
	//SDIDSTA  Data Transfer Finish (DatFin) [4 ] =1  : data finish detect
	//SDIDSTA  Data Time Out (DatTout)       [5 ] =0  : not detect,
	//SDIDSTA  Data Receive CRC Fail (DatCrc)[6 ] =0  : not detect,
	//SDIDSTA  CRC Status Fail(CrcSta)       [7 ] =0  : not detect,
	//SDIDSTA  FIFO Fail error (FFfail)      [8 ] =0  : not detect,
	//SDIDSTA  SDIO InterruptDetect(IOIntDet)[9 ] =0  : not detect,
	//SDIDSTA  Data Time Out (DatTout)       [10] =0  : not occur,
    rSDIDSTA=0x10;	// Clear data Tx/Rx end

    if(block>1)
    {
//RERCMD12: 
	    for(m=0;m<100;m++)
	    {   
			//--Stop cmd(CMD12)
			//SDICARG CmdArg [31:0] = 0 : Command Argument
			rSDICARG=0x0;	    //CMD12(stuff bit)

			//SDICCON CmdIndex              [7:0] = 0X4C : CMD12
			//SDICCON Command Start(CMST)   [8  ] = 1    : command start
			//SDICCON WaitRsp               [9  ] = 1    : wait_resp
			//SDICCON LongRsp               [10 ] = 0    : short response
			rSDICCON=(0x1<<9)|(0x1<<8)|0x4c;

			//-- Check end of CMD12
			if(SD_NO_ERR==Chk_CMDend(12, 1))return SD_NO_ERR; 
			    //goto RERCMD12;
			//rSDICSTA=0xa00;	// Clear cmd_end(with rsp)
		}
		return SD_ERR;
    }
    return SD_NO_ERR;
}

/*****************************************
  写存储块函数
  函数名: Wt_Block
  描述: 通过三种方式写存储块
  返回值：void
*****************************************/

U8 Wt_Block(U32 block,U32 *src,U32 *dst)
{
   U32 m,n;
  
	//SDICON  FIFO Reset (FRST)  [1] = 1 : FIFO reset
	
    rSDICON |= rSDICON|(1<<1);	// FIFO reset
    
	//SDICARG CmdArg [31:0] = 0 : Command Argument		
    rSDICARG=(int)(dst);	    // CMD24/25(addr)

//REWTCMD:
       //use dma
	    for(n=0;n<100;n++)
	    {
		    pISR_DMA0=(unsigned)DMA_end;
			//INTMSK INT_DMA0  [17] = 0 : Service availa
			rINTMSK = ~(BIT_DMA0);
			//DISRC0  S_ADDR  [30:0] = Tx_buffer : Tx_buffer
		    rDISRC0=(int)(src);
			//DISRCC0 INC [0] = 0 :  inc
		    //DISRCC0 LOC [1] = 0 :  AHB
		    rDISRCC0=(0<<1)+(0<<0);	// AHB, inc
			rDIDST0=(U32)(SDIDAT);	// SDIDAT
		    //DIDSTC0 INC [0] = 1 : fix
		    //DIDSTC0 LOC [1] = 1 : APB
			rDIDSTC0=(1<<1)+(1<<0);	// APB, fix

		    //DCON0 TC          [19: 0] = 128*block: Initial transfer count (or transfer beat).
		    //DCON0 DSZ 		[21:20] = 10	   : Word
		    //DCON0 RELOAD      [22   ] = 1        : auto-reload of
		    //DCON0 SWHW_SEL    [23   ] = 1        : H/W request
		    //DCON0 HWSRCSEL    [26:24] = 10       : SDI
		    //DCON0 SERVMODE    [27   ] = 0        : single service
		    //DCON0 TSZ         [28   ] = 0        : single tx
		    //DCON0 INT         [29   ] = 1		   : TC int
		    //DCON0 SYNC        [30   ] = 0        : sync PCLK
		    //DCON0 DMD_HS      [31   ] = 1        : handshake
			rDCON0=(1<<31)+(0<<30)+(1<<29)+(0<<28)+(0<<27)+(2<<24)+(1<<23)+(1<<22)+(2<<20)+128*block;

		    //DMASKTRIG0  SW_TRIG [0] = 0 : no-sw trigger 
		    //DMASKTRIG0  ON_OFF  [1] = 1 : DMA2 channel on
		    //DMASKTRIG0  STOP    [2] = 0 : no-stop
		    rDMASKTRIG0=(0<<2)+(1<<1)+0;    
		    
			//SDIDCON  BlkNum                           [11: 0] = block : Block Number (0~4095).
			//SDIDCON  Data Transfer Mode (DatMode)     [13:12] = 3     : data transmit start
			//SDIDCON  Stop by force (STOP)             [14   ] = 1     : stop by force
			//SDIDCON  DMA Enable(EnDMA)			    [15   ] = 1     : dma enable,
			//SDIDCON  Wide bus enable (WideBus)        [16   ] = 0     : standard bus mode(only SDIDAT[0] used),
			//SDIDCON  Block mode (BlkMode)  		    [17   ] = 1     : block data transfer
			//SDIDCON  Busy AfterCommand(BACMD)         [18   ] = 0     : directly after DatMode set,
			//SDIDCON  Receive After Command (RACMD)    [19   ] = 1     : Rx after cmd
			//SDIDCON  Transmit After Response(TARSP)   [20   ] = 1     : after response receive(assume DatMode sets to 2’b11)
			//SDIDCON  SDIO InterruptPeriodType(PrdType)[21   ] = 0     : exactly 2 cycle,
			rSDIDCON=(1<<20)|(1<<17)|(Wide<<16)|(1<<15)|(3<<12)|(block<<0);
			    // Tx after rsp, blk, 4bit bus, dma enable, Tx start, blk num
		    if(block<2)	    // SINGLE_WRITE
		    {
		    //SDICCON CmdIndex              [7:0] = 0X58 : CMD24
			//SDICCON Command Start(CMST)   [8  ] = 1    : command start
			//SDICCON WaitRsp               [9  ] = 1    : wait_resp
			//SDICCON LongRsp               [10 ] = 0    : short response
			rSDICCON=(0x1<<9)|(0x1<<8)|0x58;    //sht_resp, wait_resp, dat, start, CMD24
			if(SD_NO_ERR==Chk_CMDend(24, 1))	//-- Check end of CMD24
			    break;	    
		    }
		    else	    // MULTI_WRITE
		    {
		    //SDICCON CmdIndex              [7:0] = 0X59 : CMD25
			//SDICCON Command Start(CMST)   [8  ] = 1    : command start
			//SDICCON WaitRsp               [9  ] = 1    : wait_resp
			//SDICCON LongRsp               [10 ] = 0    : short response
			rSDICCON=(0x1<<9)|(0x1<<8)|0x59;    //sht_resp, wait_resp, dat, start, CMD25
			if(SD_NO_ERR==Chk_CMDend(25, 1))	//-- Check end of CMD25 
			    break;	    
		    }
	    }
	    if(n==100)return SD_ERR;

	    //rSDICSTA=0xa00;	// Clear cmd_end(with rsp)

	    while(!TR_end);		
	    //INTMSK  BIT_DMA0 [17] = 1 : MASK
	    rINTMSK |= (BIT_DMA0);
	    TR_end=0;
		//DMASKTRIG0  STOP [2] = 1 : DMA0 stop
		rDMASKTRIG0=(1<<2);	//DMA0 stop
    
    //-- Check end of DATA
    if(!Chk_DATend()) 
    {
	    Uart_Printf("dat error\n");
	    rSDIDSTA=0x10;	
	    return SD_ERR;
	}
	//SDIDSTA  Rx Data Progress On (RxDatOn) [0 ]  R  : Data receive in progress.
	//SDIDSTA  Tx Data progress On (TxDatOn) [1 ]  R  : Data transmit in progress.
	//SDIDSTA  Start Bit Error (SbitErr)     [2 ] =0  : not detect,
	//SDIDSTA  Busy Finish (BusyFin)         [3 ] =0  : not detect,
	//SDIDSTA  Data Transfer Finish (DatFin) [4 ] =1  : data finish detect
	//SDIDSTA  Data Time Out (DatTout)       [5 ] =0  : not detect,
	//SDIDSTA  Data Receive CRC Fail (DatCrc)[6 ] =0  : not detect,
	//SDIDSTA  CRC Status Fail(CrcSta)       [7 ] =0  : not detect,
	//SDIDSTA  FIFO Fail error (FFfail)      [8 ] =0  : not detect,
	//SDIDSTA  SDIO InterruptDetect(IOIntDet)[9 ] =0  : not detect,
	//SDIDSTA  Data Time Out (DatTout)       [10] =0  : not occur,
    rSDIDSTA=0x10;	

    if(block>1)
    {
	//--Stop cmd(CMD12)
//REWCMD12:
    for(m=0;m<100;m++)
    { 

		//SDIDCON  BlkNum                           [11: 0] = block : Block Number (0~4095).
	    //SDIDCON  Data Transfer Mode (DatMode)     [13:12] = 1     : only busy check start
	    //SDIDCON  Stop by force (STOP)             [14   ] = 0     : normal
	    //SDIDCON  DMA Enable(EnDMA)			    [15   ] = 0     : disable(polling),
	    //SDIDCON  Wide bus enable (WideBus)        [16   ] = 0     : standard bus mode(only SDIDAT[0] used),
	    //SDIDCON  Block mode (BlkMode)  		    [17   ] = 1     : blk 
	    //SDIDCON  Busy AfterCommand(BACMD)         [18   ] = 1     : after command sent (assume DatMode sets to 2’b01)
	    //SDIDCON  Receive After Command (RACMD)    [19   ] = 1     : Rx after cmd
	    //SDIDCON  Transmit After Response(TARSP)   [20   ] = 0     : directly after DatMode set,
	    //SDIDCON  SDIO InterruptPeriodType(PrdType)[21   ] = 0     : exactly 2 cycle,
		rSDIDCON=(1<<18)|(1<<17)|(0<<16)|(1<<12)|(block<<0);

		//--Stop cmd(CMD12)
		//SDICARG CmdArg [31:0] = 0 : Command Argument
		rSDICARG=0x0;	    //CMD12(stuff bit)

		//SDICCON CmdIndex              [7:0] = 0X4c : CMD12
		//SDICCON Command Start(CMST)   [8  ] = 1    : command start
		//SDICCON WaitRsp               [9  ] = 1    : wait_resp
		//SDICCON LongRsp               [10 ] = 0    : short response
		rSDICCON=(0x1<<9)|(0x1<<8)|0x4c;    //sht_resp, wait_resp, start, CMD12

		//-- Check end of CMD12
		if(SD_NO_ERR==Chk_CMDend(12, 1)) 
		    break;
	}
	
	if(m==100)return SD_ERR;
	//rSDICSTA=0xa00;	// Clear cmd_end(with rsp)

	//-- Check end of DATA(with busy state)
	if(!Chk_BUSYend()) 
	{
	    Uart_Printf("error\n");
	    rSDIDSTA=0x08;
	    return SD_ERR;
	}
	    
	//SDIDSTA  Rx Data Progress On (RxDatOn) [0 ]  R  : Data receive in progress.
	//SDIDSTA  Tx Data progress On (TxDatOn) [1 ]  R  : Data transmit in progress.
	//SDIDSTA  Start Bit Error (SbitErr)     [2 ] =0  : not detect,
	//SDIDSTA  Busy Finish (BusyFin)         [3 ] =1  : busy finish detect
	//SDIDSTA  Data Transfer Finish (DatFin) [4 ] =0  : not detect,
	//SDIDSTA  Data Time Out (DatTout)       [5 ] =0  : not detect,
	//SDIDSTA  Data Receive CRC Fail (DatCrc)[6 ] =0  : not detect,
	//SDIDSTA  CRC Status Fail(CrcSta)       [7 ] =0  : not detect,
	//SDIDSTA  FIFO Fail error (FFfail)      [8 ] =0  : not detect,
	//SDIDSTA  SDIO InterruptDetect(IOIntDet)[9 ] =0  : not detect,
	//SDIDSTA  Data Time Out (DatTout)       [10] =0  : not occur,
	rSDIDSTA=0x08;
	return SD_NO_ERR;
    }
    return SD_NO_ERR;
}

U8 Chk_CMDend(int cmd, int be_resp)
//0: Timeout
{
    int finish0;

    if(!be_resp)    // No response
    {	

		//SDICSTA  RspIndex                     [7:0]  R    Response index 6bit with start 2bit (8bit)
	    //SDICSTA  CMD line progress On (CmdOn) [8  ]  R    Command transfer in progress.
	    //SDICSTA  Response Receive End (RspFin)[9  ] = 1 : response end
	    //											  = 0 : not detect,
	    //SDICSTA  Command Time Out (CmdTout)   [10 ] = 1 : timeout
	    //											  = 0 : not detect
	    //SDICSTA  Command Sent (CmdSent)       [11 ] = 1 : command end
	    //											  = 0 : not detect
   		//SDICSTA  Response CRC Fail(RspCrc     [12 ] = 1 : crc fail
   		//											  = 0 : not detect
    	finish0=rSDICSTA;
	while((finish0&0x800)!=0x800)	// Check cmdend==1,
	    finish0=rSDICSTA;

	rSDICSTA=finish0;// Clear cmd end state

	return SD_NO_ERR;
    }
    else	// With response
    {
    	finish0=rSDICSTA;
	while( !( ((finish0&0x200)==0x200) | ((finish0&0x400)==0x400) ))    // Check cmd/rsp end
    	    finish0=rSDICSTA;

	if(cmd==1 | cmd==9 | cmd==41)	// CRC no check
	{
	    if( (finish0&0xf00) != 0xa00 )  // Check error
	    {
		rSDICSTA=finish0;   // Clear error state

		if(((finish0&0x400)==0x400))
		    return CMD_ERR;	// Timeout error
        }
	    rSDICSTA=finish0;	// Clear cmd & rsp end state
	}
	else	// CRC check
	{
	    if( (finish0&0x1f00) != 0xa00 )	// Check error
	    {
		Uart_Printf("CMD%d:rSDICSTA=0x%x, rSDIRSP0=0x%x, 0x%x\n",cmd, rSDICSTA, rSDIRSP0,(finish0&0x1f00));
		rSDICSTA=finish0;   // Clear error state

		if(((finish0&0x400)==0x400))
		    return CMD_ERR;	// Timeout error
    	}
	    rSDICSTA=finish0;
	}
	return SD_NO_ERR;
    }
}

int Chk_DATend(void)
{
    int finish;

    finish=rSDIDSTA;
    while( !( ((finish&0x10)==0x10) | ((finish&0x20)==0x20) ))	
	// Chek timeout or data end
	finish=rSDIDSTA;

    if( (finish&0xfc) != 0x10 )
    {
        Uart_Printf("DATA:finish=0x%x\n", finish);
        rSDIDSTA=0xec;  // Clear error state
        return 0;
    }
    return 1;
}

int Chk_BUSYend(void)
{
    int finish;

    finish=rSDIDSTA;
    while( !( ((finish&0x08)==0x08) | ((finish&0x20)==0x20) ))
	finish=rSDIDSTA;

    if( (finish&0xfc) != 0x08 )
    {
        Uart_Printf("DATA:finish=0x%x\n", finish);
        rSDIDSTA=0xf4;  //clear error state
        return 0;
    }
    return 1;
}

U8 CMD0(void)
{
    //-- Make card idle state 
	//SDICARG CmdArg [31:0] = 0 : Command Argument
	rSDICARG=0x0;	    // CMD0(stuff bit)

	//SDICCON CmdIndex              [7:0] = 0X40 : CMD0
	//SDICCON Command Start(CMST)   [8  ] = 1    : command start
	rSDICCON=(1<<8)|0x40;   
    //-- Check end of CMD0
    return (Chk_CMDend(0, 0));
    //rSDICSTA=0x800;	    // Clear cmd_end(no rsp)
}

U8 CMD55(U16 RCA)
{
    //--Make ACMD

	//SDICARG CmdArg [31:0] = 0 : Command Argument
	rSDICARG=RCA<<16;			//CMD7(RCA,stuff bit)

	//SDICCON CmdIndex              [7:0] = 0X77 : CMD55
	//SDICCON Command Start(CMST)   [8  ] = 1    : command start
	//SDICCON WaitRsp               [9  ] = 1    : wait_resp
	//SDICCON LongRsp               [10 ] = 0    : short response
	rSDICCON=(0x1<<9)|(0x1<<8)|0x77;	//sht_resp, wait_resp, start, CMD55

    //-- Check end of CMD55
    if(SD_NO_ERR!=Chk_CMDend(55, 1)) 
	return CMD_ERR;

    //rSDICSTA=0xa00;	// Clear cmd_end(with rsp)
    return SD_NO_ERR;
}



U8 Set_4bit_bus(U16 RCA)
{
    U8 ret;
    Wide=1;
    ret=SetBus(RCA);
    if(ret!=SD_NO_ERR)return ret;
    Uart_Printf("\n****4bit bus****\n");
    return ret;
}

U8 SetBus(U16 RCA)
{
    U32 n;
//SET_BUS:
    for(n=0;n<100;n++)
    {
	    if(SD_NO_ERR!=CMD55(RCA))return SD_ERR;	// Make ACMD
	    //-- CMD6 implement
		//SDICARG CmdArg [31:0] = 0 : Command Argument
	    rSDICARG=Wide<<1;	    //Wide 0: 1bit, 1: 4bit

	    //SDICCON CmdIndex              [7:0] = 0X46 : CMD55
		//SDICCON Command Start(CMST)   [8  ] = 1    : command start
		//SDICCON WaitRsp               [9  ] = 1    : wait_resp
		//SDICCON LongRsp               [10 ] = 0    : short response
		rSDICCON=(0x1<<9)|(0x1<<8)|0x46;	//sht_resp, wait_resp, start, CMD55

	    if(SD_NO_ERR==Chk_CMDend(6, 1))break;   // ACMD6
	}
	if(n==100)return SD_ERR;
	return SD_NO_ERR;
    //rSDICSTA=0xa00;	    // Clear cmd_end(with rsp)
}

void Set_Prt(void)
{
    //-- Set protection addr.0 ~ 262144(32*16*512) 
    Uart_Printf("[Set protection(addr.0 ~ 262144) test]\n");

RECMD28:
    //--Make ACMD
	//SDICARG CmdArg [31:0] = 0 : Command Argument
    rSDICARG=0;	    // CMD28(addr) 

	//SDICCON CmdIndex              [7:0] = 0X5c : CMD28
	//SDICCON Command Start(CMST)   [8  ] = 1    : command start
	//SDICCON WaitRsp               [9  ] = 1    : wait_resp
	//SDICCON LongRsp               [10 ] = 0    : short response
	rSDICCON=(0x1<<9)|(0x1<<8)|0x5c;	//sht_resp, wait_resp, start, CMD28

    //-- Check end of CMD28
    if(!Chk_CMDend(28, 1)) 
	goto RECMD28;
    //rSDICSTA=0xa00;	// Clear cmd_end(with rsp)
}

void Clr_Prt(void)
{
    //-- Clear protection addr.0 ~ 262144(32*16*512) 
    Uart_Printf("[Clear protection(addr.0 ~ 262144) test]\n");

RECMD29:
    //--Make ACMD
    //SDICARG CmdArg [31:0] = 0 : Command Argument
    rSDICARG=0;	    // CMD29(addr)

	//SDICCON CmdIndex              [7:0] = 0X5d : CMD29
	//SDICCON Command Start(CMST)   [8  ] = 1    : command start
	//SDICCON WaitRsp               [9  ] = 1    : wait_resp
	//SDICCON LongRsp               [10 ] = 0    : short response
	rSDICCON=(0x1<<9)|(0x1<<8)|0x5d;	//sht_resp, wait_resp, start, CMD29

    //-- Check end of CMD29
    if(!Chk_CMDend(29, 1)) 
	goto RECMD29;
    //rSDICSTA=0xa00;	// Clear cmd_end(with rsp)
}




//the new_funtion test

void SD_Clk400k(void)
{
   rSDIPRE=PCLK/(2*INICLK)-1;
   Uart_Printf("400k\n");

}

void SD_ClkToMax(void)
{
    rSDIPRE=PCLK/(2*NORCLK)-1;
}
void SD_Powerup(void)
{
    int i;
    rSDICON=(1<<4)|(1<<1)|1;
    rSDIBSIZE=SD_BLOCKSIZE;	
    rSDIDTIMER=0xffff;
    //rSDIIMSK =0x0;	
   
    //SDIBSIZE  BlkSize  [11:0] = 0x200 : 512byte(128word)
   	
   
    //SDIDTIMER DataTimer [15:0] = 0xffff : Data / busy timeout period (0~65535 cycle
    
   // rSDIDSTA=rSDIDSTA;		
    for(i=0;i<0x1000;i++);
    Uart_Printf("power up\n");

    
}
void SD_HardWareInit(void)
{
    
   //GPEUP  GPE[15:0]  1: The pull-up function is disabled.
    rGPEUP  = 0xf83f;     // The pull up
    //GPECON  GPE5  [11:10] = 10 :  SDCLK
    //GPECON  GPE6  [13:12] = 10 :  SDCMD
    //GPECON  GPE7  [15:14] = 10 :  SDDAT0
    //GPECON  GPE8  [17:16] = 10 :  SDDAT1
    //GPECON  GPE9  [19:18] = 10 :  SDDAT2
    //GPECON  GPE10 [21:20] = 10 :  SDDAT3
    rGPECON = 0xaaaaaaaa;
    SD_Clk400k();
    SD_Powerup();
   
}
U8   SD_ResetSD(void)
{
    
     return(CMD0());
}
U8   Card_Indentify(U16 RCA)
{
    int i;
    U8 c;

    //-- Negotiate operating condition for SD, it makes card ready state
    for(i=0;i<15;i++)
    {
    	CMD55(RCA);    // Make ACMD

    	rSDICARG=0xff8000;	//ACMD41(OCR:2.7V~3.6V)
    	rSDICCON=(0x1<<9)|(0x1<<8)|0x69;//sht_resp, wait_resp, start, ACMD41
        c=Chk_CMDend(41, 1);
	//-- Check end of ACMD41
    	if( c==SD_NO_ERR & rSDIRSP0==0x80ff8000 ) 
	    {
	      //rSDICSTA=0xa00;	// Clear cmd_end(with rsp)

	      return (U8)SD_CARD;	// Success	    
	    }
	Delay(200); // Wait Card power up status
    }
    //Uart_Printf("SDIRSP0=0x%x\n",rSDIRSP0);
    //rSDICSTA=0xa00;
   	// Clear cmd_end(with rsp)
   	if(c!=SD_NO_ERR)return (U8)CMD_ERR;
   	Uart_Printf("Card_Indentify\n");

    return (U8)MMC_CARD;	
}
     
U8   SD_ReadAllCID(void)
{
     U32 n;
     for(n=0;n<100;n++)
     {
	    //-- Check attaced cards, it makes card identification state
	    //SDICARG CmdArg [31:0] = 0 : Command Argument
	    rSDICARG=0x0;   // CMD2(stuff bit)
	   
	    //SDICCON CmdIndex              [7:0] = 0X42 : Command index with start 2bit (8bit)
	    //SDICCON Command Start(CMST)   [8  ] = 1    : command start
	    //SDICCON WaitRsp               [9  ] = 1    : wait response
	    //SDICCON LongRsp               [10 ] = 1    : long response
	    rSDICCON=(0x1<<10)|(0x1<<9)|(0x1<<8)|0x42; //lng_resp, wait_resp, start, CMD2

	    //-- Check end of CMD2
	    if(SD_NO_ERR==Chk_CMDend(2, 1))
	    {
	        Uart_Printf("\nEnd id\n");
	        return SD_NO_ERR;
	    }
      }
    //rSDICSTA=0xa00;	// Clear cmd_end(with rsp)
    return SD_ERR;
    
}  

U8   SD_GetRCA(U8 card_type,U16 *pRCA)
{
    U32 n,m;
    if(card_type==SD_CARD)
    {
	    for(n=0;n<100;n++)
	    {
		    // RECMD3:
		    //--Send RCA
		    //SDICARG   CMD3(MMC:Set RCA, SD:Ask RCA-->SBZ)
		    for(m=0;m<100;m++)
		    {
			    rSDICARG=0<<16;	   
			   
			    //SDICCON CmdIndex              [7:0] = 0X43 : Command index with start 2bit (8bit)
			    //SDICCON Command Start(CMST)   [8  ] = 1    : command start
			    //SDICCON WaitRsp               [9  ] = 1    : wait response
			    //SDICCON LongRsp               [10 ] = 0    : short response
			    rSDICCON=(0x1<<9)|(0x1<<8)|0x43;	// sht_resp, wait_resp, start, CMD3

			    //-- Check end of CMD3
			    if(SD_NO_ERR==Chk_CMDend(3, 1))break;
		    }
		    //rSDICSTA=0xa00;	// Clear cmd_end(with rsp)
		    //--Publish RCA
		    if(m==100)continue;
			*pRCA=(( rSDIRSP0 & 0xffff0000 )>>16);
		    Uart_Printf("RCA=0x%x\n",*pRCA);

		    //--State(stand-by) check
		    if((( rSDIRSP0 & 0x1e00)==0x600) )
		    {
		        return SD_NO_ERR;  // CURRENT_STATE check
		    }
	    }
	    Uart_Printf("CARD_STATUS=0x%x\n",rSDIRSP0 & 0x1e00);
        return SD_ERR;
    }
    return MMC_CARD;
	//goto RECMD3;
}

U8   SD_GetCardInfo(SDFSDisk *sds)
{

     U32 n,i,temp,sum,C_Size;
     U16 cRCA;
     cRCA=sds->RCA;
     for(n=0;n<100;n++)
     {
        
             rSDICARG =cRCA<<16;
             rSDICCON=(0x1<<10)|(0x1<<9)|(0x1<<8)|0x49;
             if(SD_NO_ERR==Chk_CMDend(9,1))break;
             
     }
     if(n==100)return SD_ERR;
     temp=(U32)((rSDIRSP1&0x000f0000)>>16);//To get blocklen
     sum=1;
     for(i=0;i<temp;i++)
     {
         sum*=2;
     }
     sds->block_len=sum;
     temp=(U32)((rSDIRSP2&0xc0000000)>>30);
     C_Size=(U32)((rSDIRSP1&0x000003ff)<<2);
     C_Size+=temp;
     temp=(U32)(((rSDIRSP2&0x00038000)>>15)+2);
     sum=1;
     for(i=0;i<temp;i++)
     {
         sum*=2;
     }
     sds->block_num=((C_Size+1)*sum);
     temp=(U32)((rSDIRSP2&0x00003f80)>>7);
     sds->sector_size=temp;
     return SD_NO_ERR;
   
}

U8   SD_SelectCard(U16 RCA)
{
     return(Card_sel_desel(RCA,1));
}
U8   SD_DeSelectCard(void)
{
     return(Card_sel_desel(0,0));
}
U8   SD_Initialize(SDFSDisk *sds)
{    
     U8 response[16],ret;
     sds->RCA=0;
     SD_HardWareInit();
     ret=SD_ResetSD();
     if(ret!=SD_NO_ERR)
     {
         Uart_Printf("reset SD err\n");
         return ret;
     }
     Uart_Printf("reset SD\n");

     ret=Card_Indentify(sds->RCA);
     if(ret==CMD_ERR)
     {
        Uart_Printf("Indentify ERR\n");
        return ret;
     }
     Uart_Printf("card_type  success\n");

     sds->card_type=ret;
     ret=SD_ReadAllCID();
     if(ret!=SD_NO_ERR)
     {
          Uart_Printf("read CID err\n");
          return ret;
     }
     if(sds->RCA==0)
     {
        card_id++;
        sds->RCA=card_id;
     }
     ret=SD_GetRCA(sds->card_type,&sds->RCA);
     if(ret!=SD_NO_ERR)
     {
          Uart_Printf("get RCA err\n");
          return ret;
     }
     ret=SD_GetCardInfo(sds);
     if(ret!=SD_NO_ERR)
     {
          Uart_Printf("Get  Card_Info err\n");
          return ret;
     }
     
     Uart_Printf("block_num=0x%x\n",FS__pDevInfo[0].harddisk_info->block_num);
     Uart_Printf("block_len=0x%x\n",FS__pDevInfo[0].harddisk_info->block_len);
     Uart_Printf("sector_size=0x%x\n",FS__pDevInfo[0].harddisk_info->sector_size);
     Uart_Printf("card_type=0x%x\n",FS__pDevInfo[0].harddisk_info->card_type);
     SD_ClkToMax();
     Uart_Printf("CLK_MAX\n");
     return SD_NO_ERR;
     
     
}
U8   SD_ReadCard_Status(U16 RCA,U8 len,U8 status[])
{
     U32 n,i;
     int response0;
	 for(n=0;n<100;n++)
	 {
	 	rSDICARG=RCA<<16;
	 	rSDICCON=(0x1<<9)|(0x1<<8)|0x4d;
	  if(SD_NO_ERR==Chk_CMDend(13,1))break;
	 }
	 if(n==100)return SD_ERR;
	 for(i=0;i<len;i++)
	 {
	 	status[i]=((rSDIRSP0&(0xff<<(i*8)))>>(i*8));
	 }
	if(rSDIRSP0&0x100)
	Uart_Printf("Ready for Data\n");
    else 
	Uart_Printf("Not Ready\n");
	response0=rSDIRSP0;
    response0 &= 0x1e00;
    response0 = response0 >> 9;
    Uart_Printf("Current Status=%d\n", response0);
	
    return SD_NO_ERR;
}

U8   SD_ReadBlock(SDFSDisk *sds,U32 blocknum,U8 *recbuf)
{
     U8 ret,status[4];
     U32 i;
     U32 *src,dst[128];
     for(i=0;i<128;i++)
     {
     	dst[i]=0x0;
     }
     src=(U32 *)(blocknum*SD_BLOCKSIZE);
     ret=SD_ReadCard_Status(sds->RCA,4,status);
     if(ret!=SD_NO_ERR)return ret;
     ret=SD_SelectCard(sds->RCA);
     if(ret!=SD_NO_ERR) return ret;
     if(sds->card_type==SD_CARD)
     {
     	 ret=Set_4bit_bus(sds->RCA);
     	 if(ret!=SD_NO_ERR)return ret;
     }
     Uart_Printf("block_LEN=0x%x\n",rSDIBSIZE);
     ret=Rd_Block((U32)1,(U32 *)src,(U32 *)dst);
     if(ret!=SD_NO_ERR)return ret;
     for(i=0;i<128;i++)
     {
     	recbuf[4*i+3]=(U8)(dst[i]&0xff);
     	recbuf[4*i+2]=(U8)((dst[i]&0xff00)>>8);
     	recbuf[4*i+1]=(U8)((dst[i]&0xff0000)>>16);
     	recbuf[4*i+0]=(U8)((dst[i]&0xff000000)>>24);
     }
     ret=SD_DeSelectCard();
     return SD_NO_ERR;
}

U8   SD_WriteBlock(SDFSDisk *sds,U32 blocknum,U8 *recbuf)
{
	 U8 ret,status[4];
	 U32 i;
	 U32 src[128],*dst;
	 for(i=0;i<128;i++)
	 {
	 	src[i]=(recbuf[4*i+0]<<24)+(recbuf[4*i+1]<<16)+(recbuf[4*i+2]<<8)+(recbuf[4*i+3]);
	 }
	 dst=(U32 *)(blocknum*SD_BLOCKSIZE);
	 ret=SD_ReadCard_Status(sds->RCA,4,status);
	  if(ret!=SD_NO_ERR) return ret;
	 ret=SD_SelectCard(sds->RCA);
	 if(ret!=SD_NO_ERR) return ret;
     if(sds->card_type==SD_CARD)
     {
     	 ret=Set_4bit_bus(sds->RCA);
     	 if(ret!=SD_NO_ERR)return ret;
     }
     Uart_Printf("block_LEN=0x%x\n",rSDIBSIZE);
     ret=Wt_Block((U32)1,(U32 *)src,(U32 *)dst);
     if(ret!=SD_NO_ERR)return ret;
     ret=SD_DeSelectCard();
     return SD_NO_ERR;
}



