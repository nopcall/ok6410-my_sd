#include <stdio.h>
#include <string.h>
#include "def.h"
#include "option.h"
#include "2410addr.h"
#include "2410lib.h"
#include "sdi.h"
#include "fs_int.h"


U8   SD_ReadBlock(U32 blocknum,U8 *recbuf)
{
     U8 ret,status[4];
     U32 i;
     U32 src,dst[128];
     TR_end=0;
     for(i=0;i<128;i++)
     {
     	dst[i]=0x0;
     }
     src=(blocknum*SD_BLOCKSIZE);
     ret=SD_ReadCard_Status((FS__pDevInfo[0].harddisk_info->RCA),4,status);
    // Uart_Printf("READ CARD RCA=0x%x\n",(FS__pDevInfo[0].harddisk_info->RCA));
     if(ret!=SD_NO_ERR)
     {
         //Uart_Printf("READ CARD STATUS FALSE\n");
         return ret;
     }
     ret=SD_SelectCard((FS__pDevInfo[0].harddisk_info->RCA));
     if(ret!=SD_NO_ERR) return ret;
     if((FS__pDevInfo[0].harddisk_info->card_type)==SD_CARD)
     {
     	 ret=Set_4bit_bus((FS__pDevInfo[0].harddisk_info->RCA));
     	 if(ret!=SD_NO_ERR)return ret;
     }
     //Uart_Printf("block_LEN=0x%x\n",rSDIBSIZE);
     ret=Rd_Block(1,src,dst);
     if(ret!=SD_NO_ERR)
     {
       // Uart_Printf("SD_readBlock false\n");
        return ret;
     }   
     for(i=0;i<128;i++)
     {
     	recbuf[4*i+3]=(U8)(dst[i]&0xff);
     	recbuf[4*i+2]=(U8)((dst[i]&0xff00)>>8);
     	recbuf[4*i+1]=(U8)((dst[i]&0xff0000)>>16);
     	recbuf[4*i+0]=(U8)((dst[i]&0xff000000)>>24);
     }
     ret=SD_DeSelectCard();
     return ret;
}

U8 Rd_Block(U32 block,U32 src,U32 *dst)
{    
    int m,n;
    U8 resp[6];

	//SDICON FIFO Reset (FRST) [1] = 1 : FIFO reset
    rSDICON |= rSDICON|SDICON_FRESET;	
        for(n=0;n<15;n++)
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
		    
		    rDIDST0=(U32)dst;	// Rx_buffer
		    
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
			rSDIDCON=SDIDCON_RACMD_1|SDIDCON_BLK|SDIDCON_WIDE|SDIDCON_DMA|SDIDCON_RX|(block<<0);
			    // Rx after rsp, blk, 4bit bus, dma enable, Rx start, blk num
		    if(block<2)	// SINGLE_READ
		    {
		   	
		        if(SD_NO_ERR==SD_SendCommand(CMD17,src,CMD17_R,resp))
		              break;
		    }
		    else	// MULTI_READ
		    {
		          if(SD_NO_ERR==SD_SendCommand(CMD18,src,CMD18_R,resp))break;
		    }
	    }
	    if(n==15)
	    {
	        //Uart_Printf("cmd17 err\n");
	        return SD_ERR;
	    }
	     //Uart_Printf("cmd17 success\n");
	    // Uart_Printf("TR_end=%d\n",TR_end);

	    //rSDICSTA=0xa00;	// Clear cmd_end(with rsp)
	    while(!TR_end);
		//Uart_Printf("rSDIFSTA=0x%x\n",rSDIFSTA);
	    
	    //INTMSK  BIT_DMA0 [17] = 1 : MASK
	    rINTMSK |= (BIT_DMA0);
	     rSDIDCON&=(~(SDIDCON_DMA));
		TR_end=0;

		//DMASKTRIG0  STOP [2] = 1 : DMA0 stop
		rDMASKTRIG0=(1<<2);	

    //-- Check end of DATA
    if(!Chk_DATend())
    { 
	   // Uart_Printf("dat error\n");
	    return SD_ERR;
	}

    if(block>1)
    {
	    for(m=0;m<15;m++)
	    {   
	       if(SD_NO_ERR==SD_SendCommand(CMD12,0x0,CMD12_R,resp))return SD_NO_ERR;
		}
		return SD_ERR;
    }
    return SD_NO_ERR;
}
