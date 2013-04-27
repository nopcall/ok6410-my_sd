

#include "def.h"
#include "option.h"
#include "2410addr.h"
#include "2410lib.h"
#include "2410slib.h"

void Isr_Init(void);
void HaltUndef(void);
void HaltSwi(void);
void HaltPabort(void);
void HaltDabort(void);

void Main(void)
{
    Port_Init();		//IO端口初始化
	Isr_Init();			//中断初始化

    Uart_Init(0,115200);//串口初始化
    Uart_Select(0);

    Uart_Printf("\n\nDM2410 Experiment System (ADS) Ver1.10\n") ;//打印系统信息
    
    Test_SDI();
    
}

//===================================================================
void Isr_Init(void)
{
    pISR_UNDEF  = (unsigned)HaltUndef;
    pISR_SWI    = (unsigned)HaltSwi;
    pISR_PABORT = (unsigned)HaltPabort;
    pISR_DABORT = (unsigned)HaltDabort;
    
    rINTMOD     = 0x0;                     //All=IRQ mode
//    rINTCON=0x5;                           //Non-vectored,IRQ enable,FIQ disable    
    rINTMSK     = BIT_ALLMSK;              //All interrupt is masked.
    rINTSUBMSK  = BIT_SUB_ALLMSK;          //All sub-interrupt is masked. <- April 01, 2002 SOP

//    rINTSUBMSK  = ~(BIT_SUB_RXD0);         //Enable Rx0 Default value=0x7ff
//    rINTMSK     = ~(BIT_UART0);            //Enable UART0 Default value=0xffffffff    
    
//    pISR_UART0=(unsigned)RxInt;            //pISR_FIQ,pISR_IRQ must be initialized
}

//===================================================================
void HaltUndef(void)
{
    Uart_Printf("Undefined instruction exception.\n");
    while(1);
}

//===================================================================
void HaltSwi(void)
{
    Uart_Printf("SWI exception.\n");
    while(1);
}

//===================================================================
void HaltPabort(void)
{
    Uart_Printf("Pabort exception.\n");
    while(1);
}

//===================================================================
void HaltDabort(void)
{
    Uart_Printf("Dabort exception.\n");
    while(1);
}

//===================================================================

