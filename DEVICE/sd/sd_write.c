#include <stdio.h>
#include <string.h>
#include "def.h"
#include "option.h"
#include "2410addr.h"
#include "2410lib.h"
#include "sdi.h"
#include "fs_int.h"

U8   SD_WriteBlock(U32 blocknum,U8 *recbuf)
{
     
	 U8 ret,status[4];
	 U32 i;
	 U32 src[128],dst;
	 TR_end=0;
	 for(i=0;i<128;i++)
	 {
	 	src[i]=(recbuf[4*i+0]<<24)+(recbuf[4*i+1]<<16)+(recbuf[4*i+2]<<8)+(recbuf[4*i+3]);
	 }
	 dst=blocknum*SD_BLOCKSIZE;
	 ret=SD_ReadCard_Status((FS__pDevInfo[0].harddisk_info->RCA),4,status);
	  if(ret!=SD_NO_ERR) return ret;
	 ret=SD_SelectCard((FS__pDevInfo[0].harddisk_info->RCA));
	 if(ret!=SD_NO_ERR)
	 {
	    // Uart_Printf("Select err\n");
	     return ret;
	 }
	 //Uart_Printf("Select sucess\n");
     if((FS__pDevInfo[0].harddisk_info->card_type)==SD_CARD)
     {
     	 ret=Set_4bit_bus((FS__pDevInfo[0].harddisk_info->RCA));
     	 if(ret!=SD_NO_ERR)return ret;
     }
     //Uart_Printf("block_LEN=0x%x\n",rSDIBSIZE);
     ret=Wt_Block(1,src,dst);
     if(ret!=SD_NO_ERR)return ret;
     ret=SD_DeSelectCard();
     return ret;
}

/*****************************************
  写存储块函数
  函数名: Wt_Block
  描述: 通过三种方式写存储块
  返回值：void
*****************************************/

U8 Wt_Block(U32 block,U32 *src,U32 dst)
{
   U32 m,n;
   U8 resp[6];
  
	//SDICON  FIFO Reset (FRST)  [1] = 1 : FIFO reset
	
    rSDICON |= rSDICON|SDICON_FRESET;	// FIFO reset
	    for(n=0;n<15;n++)
	    {
		    pISR_DMA0=(unsigned)DMA_end;
			//INTMSK INT_DMA0  [17] = 0 : Service availa
			rINTMSK = ~(BIT_DMA0);
			//DISRC0  S_ADDR  [30:0] = Tx_buffer : Tx_buffer
		    rDISRC0=(U32)(src);
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
		    //DMASKTRIG0  ON_OFF  [1] = 1 : DMA0 channel on
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
			rSDIDCON=SDIDCON_TARSP_1|SDIDCON_BLK|SDIDCON_WIDE|SDIDCON_DMA |SDIDCON_TX|(block<<0);
			    // Tx after rsp, blk, 4bit bus, dma enable, Tx start, blk num
		    if(block<2)	    // SINGLE_WRITE
		    {
				if(SD_NO_ERR == SD_SendCommand(CMD24,dst,CMD24_R,resp))	
				    break;	    
		    }
		    else	    // MULTI_WRITE
		    {
		         if(SD_NO_ERR == SD_SendCommand(CMD25,dst,CMD25_R,resp))
		            break;	
		    }
	    }
	    if(n==15)
	    {
	        //Uart_Printf("cmd24 err\n");
	        return SD_ERR;
	    }
       // Uart_Printf("cmd24 success\n");
	    //rSDICSTA=0xa00;	// Clear cmd_end(with rsp)
     
	    while(!TR_end);
	    //INTMSK  BIT_DMA0 [17] = 1 : MASK
	    rINTMSK |= (BIT_DMA0);
	    rSDIDCON&=(~(SDIDCON_DMA));
	    
	    TR_end=0;
		//DMASKTRIG0  STOP [2] = 1 : DMA0 stop
		rDMASKTRIG0=(1<<2);	//DMA0 stop
    
    //-- Check end of DATA
    if(!Chk_DATend()) 
    {
	    Uart_Printf("dat error\n");
	    return SD_ERR;
	}
    if(block>1)
    {
	    for(m=0;m<15;m++)
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

			if(SD_NO_ERR==SD_SendCommand(CMD12,0x0,CMD12_R,resp)) 
			    break;
		}
	
	if(m==15)return SD_ERR;
	if(!Chk_BUSYend()) 
	{
	    Uart_Printf("error\n");
	    return SD_ERR;
	}
	return SD_NO_ERR;
    }
    return SD_NO_ERR;
}
