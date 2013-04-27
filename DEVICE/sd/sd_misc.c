#include <stdio.h>
#include <string.h>
#include "def.h"
#include "option.h"
#include "2410addr.h"
#include "2410lib.h"
#include "sdi.h"
#include "fs_int.h"

/***********************************/
/*********glocbal data**************/
/************************************/

SDFSDisk SanDisk_info_table={
         0,
         NO_CARD,
         0,
         0,
         0,
         0,
         0,
         0,
         
 };

const SDDriver  SanDisk_driver_table={

		 SD_ReadBlock,
		 SD_WriteBlock,
		 0,
		 0,
		 
};

volatile unsigned int TR_end=0;

U8 SD_SendCommand(U8 cmd, U32 param, U8 resptype, U8 *resp)
{
	U32 resp_len, stat = 0;

	stat = SDICCON_START | cmd; /* 发送命令头和命令字 send command header and word */
	rSDICARG = param;
	/* 发送参数 send parameter */
	resp_len = 0;
	switch (resptype) /* 根据不同的命令,得到不同的响应长度 */
	{ /* according various command,get the various response length */
		case R0:
			resp_len = 0; 
		break;

		case R1:
		case R1B:
		case R3:
		case R6:
			stat |= SDICCON_WRSP; /* SDICCON_WRSP:等待响应. SDICCON_WRSP: must wait for response */
			resp_len = 6;
		break;

		case R2: 
			stat |= SDICCON_WRSP;
			stat |= SDICCON_LRSP; /* SDICCON_LRSP:长响应. SDICCON_LRSP: long response */
			resp_len = 17;
		break;

		default:
			return SD_ERR; /* 返回命令响应类型错误 return error of command response type */
		break;
	}
	rSDICCON = stat; /* 向SD/MMC卡发送命令 send command to sd/mmc card */

	/* no response */
	if (resp_len == 0)
	{
		do
		{ /* recycle until the command have been sent */
			stat = rSDICSTA;
		}while (!(stat & SDICSTA_SENT)); 

		rSDICSTA = stat; /* clear bits */ 
		return SD_NO_ERR; /* 返回执行成功 return perform sucessfully */
	}

	/* there is response */
	while(1) 
	{
		stat = rSDICSTA;
		if (stat & SDICSTA_TOUT)
		{ /* time is out */
			rSDICSTA = stat; /* clear bits */
			return CMD_ERR; /* 返回命令超时 return response timeout of command */
		}

		if (stat & SDICSTA_RSP) /* 接收到响应 receive the response */
		{
			break;	
		}
		
	}
	rSDICSTA = stat; /* clear bits */ 

	/* read the response */
	if (resp_len == 6)
	{
		*resp = (U8)(rSDIRSP1 >> 24); 
		*(resp+4)=(U8)(rSDIRSP0&0xff);
	    *(resp+3)=(U8)((rSDIRSP0&0xff00)>>8);
	    *(resp+2)=(U8)((rSDIRSP0&0xff0000)>>16);
	    *(resp+1)=(U8)((rSDIRSP0&0xff000000)>>24);
	}
	else
	{
	   
		*(resp+15)=(U8)(rSDIRSP0&0xff);
	    *(resp+14)=(U8)((rSDIRSP0&0xff00)>>8);
	    *(resp+13)=(U8)((rSDIRSP0&0xff0000)>>16);
	    *(resp+12)=(U8)((rSDIRSP0&0xff000000)>>24);
		*(resp+11)=(U8)(rSDIRSP1&0xff);
	    *(resp+10)=(U8)((rSDIRSP1&0xff00)>>8);
	    *(resp+9)=(U8)((rSDIRSP1&0xff0000)>>16);
	    *(resp+8)=(U8)((rSDIRSP1&0xff000000)>>24);
		*(resp+7)=(U8)(rSDIRSP2&0xff);
	    *(resp+6)=(U8)((rSDIRSP2&0xff00)>>8);
	    *(resp+5)=(U8)((rSDIRSP2&0xff0000)>>16);
	    *(resp+4)=(U8)((rSDIRSP2&0xff000000)>>24);
		*(resp+3)=(U8)(rSDIRSP3&0xff);
	    *(resp+2)=(U8)((rSDIRSP3&0xff00)>>8);
	    *(resp+1)=(U8)((rSDIRSP3&0xff0000)>>16);
	    *(resp+0)=(U8)((rSDIRSP3&0xff000000)>>24);
	    
	} 

	return SD_NO_ERR; /* 返回执行成功 return perform sucessfully */
}

U8 Set_4bit_bus(U16 RCA)
{
    U32 n;
    U8 resp[6];
    for(n=0;n<15;n++)
    {
	    if(SD_NO_ERR!=SD_SendCommand(CMD55, (RCA<<16), CMD55_R, resp))continue;	
	    if(SD_NO_ERR==SD_SendCommand(ACMD6, (1<<1), ACMD6_R, resp))
	    {
	    	return SD_NO_ERR;
	    }
	}
    return SD_ERR;
}


U8   SD_SelectCard(U16 RCA)
{
     U32 m,n;
     U8 resp[6];
     for(n=0;n<10;n++)
	 {
	       for(m=0;m<10;m++)
	       {
				if(SD_NO_ERR==SD_SendCommand(CMD7, (RCA<<16), CMD7_R, resp))
				{
				  // break;
				// Uart_Printf("CMD7_success\n");
				 return SD_NO_ERR;
				}
		   }
            //Uart_Printf("CMD7 successr!!SDIRSP0&0x1e00=0x%x\n",( rSDIRSP0 & 0x1e00 ));
		  // if( rSDIRSP0 & 0x1e00==0x800 )return SD_NO_ERR;
	  }
	  return SD_ERR;
}
U8   SD_DeSelectCard(void)
{
     U32 m;
     for(m=0;m<10;m++)
	 {
		 if(SD_NO_ERR==SD_SendCommand(CMD7, (0<<16), R0,NULL))
		   return SD_NO_ERR;
     }
     return SD_ERR;
}

U8   SD_ReadCard_Status(U16 RCA,U8 len,U8 status[])
{
     U32 n,i;
     int response0;
	 for(n=0;n<15;n++)
	 {
	  if(SD_NO_ERR==SD_SendCommand(CMD13, (RCA<<16), CMD13_R,status))break;
	 }
	 if(n==15)return SD_ERR;
	 for(i=0;i<len;i++)
	 {
	 	status[i]=((rSDIRSP0&(0xff<<(i*8)))>>(i*8));
	 }
	if(!(rSDIRSP0&0x100))
	   Uart_Printf("Not Ready\n");
	response0=rSDIRSP0;
    response0 &= 0x1e00;
    response0 = response0 >> 9;
    //Uart_Printf("Current Status=%d\n", response0);
	
    return SD_NO_ERR;
}

int Chk_DATend(void){
    U32 finish;
    while(1){
      finish=rSDIDSTA;
      if(finish&SDIDSTA_TOUT){
         rSDIDSTA=finish;
         Uart_Printf("DATA ERR:finish=0x%x\n", finish);
         return 0;
      }
      if(finish&SDIDSTA_DFIN){
         rSDIDSTA=finish;
         return 1;
      }
    }
}


int Chk_BUSYend(void){
    U32 finish;
    while(1){
      finish=rSDIDSTA;
      if(finish&SDIDSTA_BFIN){
         rSDIDSTA=finish;
         return 1;
      }
      if(finish&SDIDSTA_TOUT){
      	rSDIDSTA=finish;
      	return 0;
      }
    }
}

void __irq DMA_end(void)
{
    ClearPending(BIT_DMA0);
    
    TR_end=1;
}
