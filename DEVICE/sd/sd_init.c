#include <stdio.h>
#include <string.h>
#include "def.h"
#include "option.h"
#include "2410addr.h"
#include "2410lib.h"
#include "sdi.h"
#include "fs_int.h"

static U32 card_id = 0;




static void SD_Clk400k(void)
{
   rSDIPRE=PCLK/(2*INICLK)-1;
   Uart_Printf("400k\n");

}

static void SD_ClkToMax(void)
{
    rSDIPRE=PCLK/(2*NORCLK)-1;
}
static void SD_Powerup(void)
{
    int i;
    rSDICON=SDICON_LE|(SDICON_FRESET)|SDICON_ENCLK;
    rSDIBSIZE=SD_BLOCKSIZE;	
    rSDIDTIMER=0xffff;
    //rSDIIMSK =0x0;	
   
    //SDIBSIZE  BlkSize  [11:0] = 0x200 : 512byte(128word)
   	
   
    //SDIDTIMER DataTimer [15:0] = 0xffff : Data / busy timeout period (0~65535 cycle
    
   // rSDIDSTA=rSDIDSTA;		
    for(i=0;i<0x1000;i++);
    Uart_Printf("power up\n");

    
}
static void SD_HardWareInit(void)
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
static U8   SD_ResetSD(void)
{
    
     return( SD_SendCommand(CMD0, 0x0, CMD0_R, NULL));
     
}
static U8   Card_Indentify(void)
{
    int i;
    U8 resp[6];
    U32 resp1;

    for(i=0;i<15;i++)
    {
    	if(SD_NO_ERR!=SD_SendCommand(CMD55, 0x0, CMD55_R, resp))continue; 
    	  
        if(SD_NO_ERR!=SD_SendCommand(ACMD41, 0xff8000, ACMD41_R, resp))continue;
        
        resp1=(resp[1]<<24)+(resp[2]<<16)+(resp[3]<<8)+resp[4];
        
    	if(resp1==0x80ff8000 ) 
	    {
	         Uart_Printf("Card_Indentify\n");
	         
             return (U8)SD_CARD;	
	    }
	    Delay(200); // Wait Card power up status
    }
     Uart_Printf("Card_Indentify false\n");
   	if(i==15)return (U8)CMD_ERR;
   	
    return (U8)MMC_CARD;	
}
     
static U8   SD_ReadAllCID(void)
{
     U32 n;
     U8 resp[18];
     for(n=0;n<15;n++)
     {
	    if(SD_NO_ERR==SD_SendCommand(CMD2, 0x0, CMD2_R, resp))
	   	{
	        Uart_Printf("\nEnd id\n");
	        return SD_NO_ERR;
	    }
     }
    return SD_ERR;
}  

static U8   SD_GetRCA(U8 card_type,U16 *pRCA)
{
    U32 n,m;
    U8 resp[6];
    if(card_type==SD_CARD)
    {
	    for(n=0;n<15;n++)
	    {
		
		    for(m=0;m<15;m++)
		    {
			    if(SD_NO_ERR==SD_SendCommand(CMD3, 0x0, CMD3_R, resp))break;
		    }
		    if(m==15)continue;
			*pRCA=(( rSDIRSP0 & 0xffff0000 )>>16);
		    Uart_Printf("RCA=0x%x\n",*pRCA);

		    //--State(stand-by) check
		    if((( rSDIRSP0 & 0x1e00)==0x600) )
		    {
		        return SD_NO_ERR;  
		    }
	    }
	    Uart_Printf("CARD_STATUS=0x%x\n",rSDIRSP0 & 0x1e00);
        return SD_ERR;
    }
    return MMC_CARD;
}

static U8   SD_GetCardInfo(SDFSDisk *sds)
{

     U32 n,i,temp,sum,C_Size;
     U8  resp[18];
     U16 cRCA;
     cRCA=sds->RCA;
     for(n=0;n<15;n++)
     {
            
          if(SD_NO_ERR== SD_SendCommand(CMD9, (cRCA<<16), CMD9_R, resp))break;
             
     }
     if(n==15)return SD_ERR;
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


U8   SD_Initialize(SDFSDisk *sds)
{    
     U8 ret;
     SD_HardWareInit();
     ret=SD_ResetSD();
     if(ret!=SD_NO_ERR)
     {
         Uart_Printf("reset SD err\n");
         return ret;
     }
     Uart_Printf("reset SD\n");

     ret=Card_Indentify();
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
     SD_ClkToMax();
     Uart_Printf("CLK_MAX\n");
     return SD_NO_ERR;
     
     
}

