#include <stdio.h>
#include <string.h>
#include "def.h"
#include "option.h"
#include "2410addr.h"
#include "2410lib.h"
#include "sdi.h"
#include "fs_int.h"
#include "fs_fat.h"
#include "fs_api.h"

S8 my_buff1[1030];
S8 my_buff2[1030];
S8 my_buff3[2410];
void new_buff(void){
  int i;
  for(i=0;i<1030;i++){
    my_buff1[i]='c';
    my_buff2[i]='d';
    }
    
}
void Test_SDI(void)
{
    U32 p,tempOffset,k;
    S32 counter;
    U8 err;
    FS_FILE *test;
    Uart_Printf("\n[SDI test]\n");
    
    err=SD_Initialize(FS__pDevInfo[0].harddisk_info);
    if(err!=SD_NO_ERR)
    {
          Uart_Printf("SD_Initialize false\n");
          return;
    }
    Uart_Printf("SD_Initialize success\n");
    Uart_Printf("block_num=0x%x\n",FS__pDevInfo[0].harddisk_info->block_num);
    Uart_Printf("block_len=0x%x\n",FS__pDevInfo[0].harddisk_info->block_len);
    Uart_Printf("sector_size=0x%x\n",FS__pDevInfo[0].harddisk_info->sector_size);
    Uart_Printf("card_type=0x%x\n",FS__pDevInfo[0].harddisk_info->card_type);
    Uart_Printf("RCA=0x%x\n",FS__pDevInfo[0].harddisk_info->RCA);
    
    
      FS_Init();
	  err= FS__fat_checkunit();
	  if(err!=FS_NO_ERR)
	  {
	  		Uart_Printf("cheack  false\n");
	  }
	  Uart_Printf("cheack success\n");
	  Uart_Printf("FS__pDiskInfo[0].SecPerClus=0x%x\n",FS__pDiskInfo[0].SecPerClus);
	  Uart_Printf("FS__pDiskInfo[0].NumFATs=0x%x\n",FS__pDiskInfo[0].NumFATs);
	  Uart_Printf("FS__pDiskInfo[0].BytsPerSec=0x%x\n",FS__pDiskInfo[0].BytsPerSec);
	  Uart_Printf(" FS__pDiskInfo[0].FATType=0x%x\n", FS__pDiskInfo[0].FATType);
	  Uart_Printf("FS__pDiskInfo[0].FATStartSec=0x%x\n",FS__pDiskInfo[0].FATStartSec);
	   new_buff();
	  for(k=0;k<500;k++){
	     int a;
	     for(a=0;a<2410;a++){
	       my_buff3[a]=0;
	     }
	     //第一个*********************************************
	     
	     
		  test=FS_FOpen("\\my\\myfile.txt","a+");
		  if(test==0){
		     Uart_Printf("Open  false\n");
		     return;
		  }
		   Uart_Printf("Open success\n");
		   tempOffset=FS_FTell(test);
		   counter=FS_FWrite(my_buff1,1,1030,test);
		   Uart_Printf("first writer counter=%d\n",counter);
		   counter=FS_FWrite(my_buff2,1,1030,test);
		   Uart_Printf("second writer counter=%d\n",counter);
		   
		   FS_FSeek(test,tempOffset,FS_SEEK_SET);
		   counter=FS_FRead(my_buff3,1,2040,test);
		   Uart_Printf("read counter=%d\n",counter);
		   for(p=1000;p<1060;p++){
		     Uart_Printf("%c",my_buff3[p]);
		   }
		   FS_FClose(test);
		  //第二个**************************************
		  
		  
		  test=FS_FOpen("\\my\\myfile1.txt","a+");
		  if(test==0){
		     Uart_Printf("Open  false\n");
		     return;
		  }
		   Uart_Printf("Open success\n");
		   tempOffset=FS_FTell(test);
		   counter=FS_FWrite(my_buff1,1,1030,test);
		   Uart_Printf("first writer counter=%d\n",counter);
		   counter=FS_FWrite(my_buff2,1,1030,test);
		   Uart_Printf("second writer counter=%d\n",counter);
		   
		   FS_FSeek(test,tempOffset,FS_SEEK_SET);
		   counter=FS_FRead(my_buff3,1,2040,test);
		   Uart_Printf("read counter=%d\n",counter);
		   for(p=1000;p<1060;p++){
		     Uart_Printf("%c",my_buff3[p]);
		   }
		   FS_FClose(test);
		  //第三个*************************************
		  
		  
		  test=FS_FOpen("\\my\\myfile2.txt","a+");
		  if(test==0){
		     Uart_Printf("Open  false\n");
		     return;
		  }
		   Uart_Printf("Open success\n");
		   tempOffset=FS_FTell(test);
		   counter=FS_FWrite(my_buff1,1,1030,test);
		   Uart_Printf("first writer counter=%d\n",counter);
		   counter=FS_FWrite(my_buff2,1,1030,test);
		   Uart_Printf("second writer counter=%d\n",counter);
		   
		   FS_FSeek(test,tempOffset,FS_SEEK_SET);
		   counter=FS_FRead(my_buff3,1,2040,test);
		   Uart_Printf("read counter=%d\n",counter);
		   for(p=1000;p<1060;p++){
		     Uart_Printf("%c",my_buff3[p]);
		   }
		   FS_FClose(test);
		   //第四个************************************
		   
		   
		  test=FS_FOpen("\\my\\myfile3.txt","a+");
		  if(test==0){
		     Uart_Printf("Open  false\n");
		     return;
		  }
		   Uart_Printf("Open success\n");
		   tempOffset=FS_FTell(test);
		   counter=FS_FWrite(my_buff1,1,1030,test);
		   Uart_Printf("first writer counter=%d\n",counter);
		   counter=FS_FWrite(my_buff2,1,1030,test);
		   Uart_Printf("second writer counter=%d\n",counter);
		   
		   FS_FSeek(test,tempOffset,FS_SEEK_SET);
		   counter=FS_FRead(my_buff3,1,2040,test);
		   Uart_Printf("read counter=%d\n",counter);
		   for(p=1000;p<1060;p++){
		     Uart_Printf("%c",my_buff3[p]);
		   }
		   FS_FClose(test);
		   //第五个*********************************
		   
		   
		  test=FS_FOpen("\\my\\myfile4.txt","a+");
		  if(test==0){
		     Uart_Printf("Open  false\n");
		     return;
		  }
		   Uart_Printf("Open success\n");
		   tempOffset=FS_FTell(test);
		   counter=FS_FWrite(my_buff1,1,1030,test);
		   Uart_Printf("first writer counter=%d\n",counter);
		   counter=FS_FWrite(my_buff2,1,1030,test);
		   Uart_Printf("second writer counter=%d\n",counter);
		   
		   FS_FSeek(test,tempOffset,FS_SEEK_SET);
		   counter=FS_FRead(my_buff3,1,2040,test);
		   Uart_Printf("read counter=%d\n",counter);
		   for(p=1000;p<1060;p++){
		     Uart_Printf("%c",my_buff3[p]);
		   }
		   FS_FClose(test);
	   }
	   FS_Exit();
}



