 /**
  ******************************************************************************
  * @file    bsp_xxx.c
  * @author  STMicroelectronics
  * @version V1.0
  * @date    2013-xx-xx
  * @brief   spi flash 底层应用函数bsp 
  ******************************************************************************
  * @attention
  *
  * 实验平台:野火 ISO-MINI STM32 开发板 
  * 论坛    :http://www.chuxue123.com
  * 淘宝    :http://firestm32.taobao.com
  *
  ******************************************************************************
  */
#include "sys_usart.h"
#include "usr_usart.h"
#include "minos_delay.h"
#include "bsp_spi_flash.h"/*******************************************************************************
* Module        : SPI1_DMA_Configuration
* Description   : SPI1以及对应DMA通道驱动函数
* Compile       : Keil MDK 4.10
* Device        ：STM32F103RBT6
* Author        : 林阿呆   (修改2012/4/30)
* Modefied data : 2012/3/8 
* Version       ：V1.0
* Copyright(C) 林阿呆 20012-2022	 
* All rights reserved
*******************************************************************************/



vu8 SPI1_RX_Buff[buffersize] = { 0 };    //接收缓冲区初始化       
vu8 SPI1_TX_Buff[buffersize] = {0x55 };	 //发送缓冲区初始化
//原版的发送的定义和初始化在接收的前面，使得主机在450（约）以上的传输时出现接收不到或者接
//收出错的问题，不知道后来改了缓冲区大小之后会不会出现同种情况，反正将其调换之后，在增大缓
//冲区就没问题了
//vu8=volatile unsigned char,volatile类型修饰符是被设计用来修饰被不同线程访问和修改的变量
							  
/*******************************************************************************
* Function Name  : SPI1_Configuration
* Description    : SPI模块包括相关IO口的配置，SPI1配置为低速，即Fsck=Fcpu/256
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/

void SPI1_Configuration( void )
{	 
  GPIO_InitTypeDef  GPIO_InitStructure;
  SPI_InitTypeDef  SPI_InitStructure;
	
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能GPIOA时钟
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);//使能SPI1时钟

  //GPIOFB3,4,5初始化设置
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;//PB3~5复用功能输出	
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
  GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA, GPIO_Pin_4);
	
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource5,GPIO_AF_SPI1); //PB3复用为 SPI1
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource6,GPIO_AF_SPI1); //PB4复用为 SPI1
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource7,GPIO_AF_SPI1); //PB5复用为 SPI1
	
	/* 配置SPI硬件参数 */
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;	/* 数据方向：2线全双工 */
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;		/* STM32的SPI工作模式 ：主机模式 */
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;	/* 数据位长度 ： 8位 */
	/* SPI_CPOL和SPI_CPHA结合使用决定时钟和数据采样点的相位关系、
	   本例配置: 总线空闲是高电平,第2个边沿（上升沿采样数据)
	*/
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;			/* 时钟上升沿采样数据 */
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;		/* 时钟的第2个边沿采样数据 */
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;			/* 片选控制方式：软件控制 */
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;	/* 波特率预分频系数：4分频 */
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;	/* 数据位传输次序：高位先传 */
	SPI_InitStructure.SPI_CRCPolynomial = 7;			/* CRC多项式寄存器，复位后为7。本例程不用 */
	SPI_Init(SPI1, &SPI_InitStructure);

    // Enable DMA1 Channel3 //
    DMA_Cmd(DMA1_Stream4, ENABLE);

	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, ENABLE);

	SPI_Cmd(SPI1, ENABLE);		/* 使能SPI  */
    

} 

  
/*******************************************************************************
* Function Name  : SPI1_SetSpeed
* Description    : SPI设置速度为高速
* Input          : u8 SpeedSet 
*                  如果速度设置输入0，则低速模式，非0则高速模式
*                  SPI_SPEED_HIGH   1
*                  SPI_SPEED_LOW    0
×                  注意设置速度会先关闭SPI，所以最好在程序发送接收完后改
*                  一般接收发送数据都有等待接收完成，所以这里不加
* Output         : None
* Return         : None
*******************************************************************************/

void SPI1_SetSpeed( u8 SpeedSet )
{	
	SPI1->CR1 &= 0XFF87 ;                                    //关闭SPI1,Fsck=Fcpu/2

	switch( SpeedSet )
	{
	    case SPI_SPEED_2   : SPI1->CR1 |= 0<<3 ; break ;     //2分频:Fsck=Fpclk/2=36Mhz;
		case SPI_SPEED_4   : SPI1->CR1 |= 1<<3 ; break ;     //4分频:Fsck=Fpclk/4=18Mhz;  
		case SPI_SPEED_8   : SPI1->CR1 |= 2<<3 ; break ;     //8分频:Fsck=Fpclk/8=9Mhz
		case SPI_SPEED_16  : SPI1->CR1 |= 3<<3 ; break ;     //16分频:Fsck=Fpclk/16=4.5Mhz
		case SPI_SPEED_32  : SPI1->CR1 |= 4<<3 ; break ;     //32分频:Fsck=Fpclk/16=2.25Mhz
		case SPI_SPEED_64  : SPI1->CR1 |= 5<<3 ; break ;     //64分频:Fsck=Fpclk/16=1.13Mhz
		case SPI_SPEED_128 : SPI1->CR1 |= 6<<3 ; break ;     //128分频:Fsck=Fpclk/16=561.5khz
		case SPI_SPEED_256 : SPI1->CR1 |= 7<<3 ; break ;     //256分频:Fsck=Fpclk/256=281.25Khz 低速模式
		default            : SPI1->CR1 |= 7<<3 ; break ;     //256分频:Fsck=Fpclk/256=281.25Khz 低速模式
	}

	SPI1->CR1 |= 1<<6 ;                                      //SPI设备使能	  
}

/*******************************************************************************
* Function Name  : SPI1_DMA_Configuration
* Description    : 配置SPI1_RX的DMA通道2，SPI1_TX的DMA通道3
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/

void SPI1_DMA_Configuration( void )
{

DMA_InitTypeDef  DMA_InitStructure;

//	//发送通道
//	DMA_DeInit(DMA1_Stream3);
//	DMA_InitStructure.DMA_PeripheralBaseAddr =(u32)&SPI1->DR;//SPI1的DR寄存器地址
//	DMA_InitStructure.DMA_Memory0BaseAddr = (u32)SPI1_TX_Buff;
//	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;  //外设地址是目的地
//	DMA_InitStructure.DMA_BufferSize = 1;
//	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
//	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  //DMA内存地址自动增加模式
//	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
//	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
//	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;    //循环模式
//	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
//	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
//  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
//  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;//存储器突发单次传输
//  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;//外�
//	DMA_Init(DMA1_Stream3, &DMA_InitStructure);

//	DMA_Cmd(DMA1_Stream3,ENABLE);

	//接收通道
	DMA_DeInit(DMA1_Stream4);
	DMA_InitStructure.DMA_PeripheralBaseAddr =(u32)&SPI1->DR;//SPI2的DR寄存器地址
	DMA_InitStructure.DMA_Memory0BaseAddr = (u32)SPI1_RX_Buff;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;  //外设地址是目的地
	DMA_InitStructure.DMA_BufferSize = 10;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  //DMA内存地址自动增加模式
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;    //循环模式
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
	DMA_Init(DMA1_Stream4, &DMA_InitStructure);

	DMA_Cmd(DMA1_Stream4,ENABLE);
} 
//设置DMA存储器地址，注意MSIZE				 




/*----------------------------------------------------------------------
发送和接收，任意数量信息（<=512Byte），length是信息长度，单位是字节，
若要增大传输数据量的话，要设置buffersize

------------------------------------------------------------------------*/
void SPI1_ReceiveSendByte(u8 *message,u16 length )
{
	u16 i=0;
 DMA_Cmd(DMA1_Stream3,DISABLE);
 DMA_Cmd(DMA1_Stream4,DISABLE);

	for(i=0;i<length;i++)					    //将所要发送的数据转到发送缓冲区上
	{
		SPI1_TX_Buff[i] =message[i] ;	
	}

	DMA1_Stream3->NDTR = 0x0000   ;           //传输数量寄存器清零
	DMA1_Stream3->NDTR = length ;             //传输数量设置为buffersize个

	DMA1_Stream4->NDTR = 0x0000   ;           //传输数量寄存器清零
	DMA1_Stream4->NDTR = length ;             //传输数量设置为buffersize个

  DMA_ClearFlag(DMA1_Stream3,DMA_IT_TCIF3);      //清DMA发送完成标志	DMA1->LIFCR = 0xF00 ;                        //清除通道3的标志位
  DMA_ClearFlag(DMA1_Stream4,DMA_IT_TCIF4);
	
	SPI1->DR ;									//接送前读一次SPI1->DR，保证接收缓冲区为空

	while( ( SPI1->SR & 0x02 ) == 0 );
	
 DMA_Cmd(DMA1_Stream3,ENABLE);
 DMA_Cmd(DMA1_Stream4,ENABLE);

  delay_ms(10);
}