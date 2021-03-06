#include <stdio.h>
#include "MPU6050.h"
#include "ADNS3080.h"
#include "spi3080.h"
#include "srom.h"
#include "usr_usart.h"
#include "rc.h"
#include "stm32f4xx_it.h"
#define H  1000 //毫米
  int sum_x,sum_y;
	
void ADNS_3080_GPIO_Configuration(void)
{

}

void ADNS3080_reset(void)  //ADNS3080 复位（高）
{
  GPIO_ResetBits(GPIOB,GPIO_Pin_13); 
  delay_ms(5);
  GPIO_SetBits(GPIOB,GPIO_Pin_13);
  delay_ms(5);
  GPIO_ResetBits(GPIOB,GPIO_Pin_13);//脉冲信号
}

void Write_srom(void)
{
 int i;
 ON_CS(); 
 writr_register(0x20,0x44);
 delay_us(51);
 writr_register(0x23,0x07);
  delay_us(51);
 writr_register(0x24,0x88);
 delay_us(51);
  OFF_CS();  //突发_写模式
  delay_us(340);//等待大于1帧时间
  ON_CS(); 
  writr_register(SROM_Enable,0x18);
  OFF_CS();  //突发_写模式
 delay_us(41);//  >40us
  ON_CS();
  for(i=0;i<=1985;i++)
  {
     writr_register(0x60,SROM[i]);
     delay_us(11);// >10us
  }
 OFF_CS();
 delay_us(105);	//>104us
}
void ADNS3080_Init(void)
{
  ADNS_3080_GPIO_Configuration();
  SPI_init(256);  //改变速度（2到256分频）
  ADNS3080_reset(); //复位
  GPIO_SetBits(GPIOA,GPIO_Pin_10);  //拉高NPD,免睡眠
  delay_ms(10);
  Write_srom();
  ADNS_Configuration();
 //printf("%d\n",read_register(0x1f));	 //查看是否下载成功
}

void ADNS_Configuration(void)
{
	 ON_CS(); 
	 writr_register(Configuration_bits,0x10);		//设置分辨率 1600	 //若Bit 4为0，则为400点每英寸
	 delay_ms(3);
	 writr_register(Extended_Config,0x01);
	 delay_ms(3);
 if(read_busy()!=1)
 {  							      //设为3000帧每秒
    OFF_CS();  //突发_写模式
	delay_ms(2);
	ON_CS();	
 	SPI_SendReceive(Frame_Period_Max_Bound_Lower+0x80);	//设置帧率 //先写低位再写高位
    SPI_SendReceive(0x40); //   C0 5000帧率	   
	SPI_SendReceive(Frame_Period_Max_Bound_Upper+0x80);
	SPI_SendReceive(0x1f);	 // 12
 } 
 clear_motion();
 OFF_CS();
}

u8 read_register(u8 adress)
{
u8 temp;
ON_CS();
temp=SPI_SendReceive(adress+0x00);	//读
delay_us(75);
temp=SPI_SendReceive(0xff);	//提供时钟信号_读
OFF_CS();
return temp;
}

void writr_register(u8 adress,u8 vlue)
{
ON_CS();
 SPI_SendReceive(adress+0x80);
 SPI_SendReceive(vlue);
OFF_CS();
delay_us(51);
}

u8 read_busy(void)//写帧率的判忙  ==1忙
{
u8 temp;
ON_CS();
temp=SPI_SendReceive(Extended_Config+0x00);
delay_us(75);
temp=SPI_SendReceive(0xff);
temp&=0x80;
OFF_CS();
return temp;
}

void clear_motion(void)
{
ON_CS();
 SPI_SendReceive(Motion_Clear+0x80);
 SPI_SendReceive(0xff);	//清除X Y数据
OFF_CS();
}


u16 read_zhenlv(void) //读帧率
{
  u16 Frame_Period_Max_Bound_Lower1,Frame_Period_Max_Bound_Upper1;
  ON_CS();
  Frame_Period_Max_Bound_Upper1=SPI_SendReceive(Frame_Period_Uppe+0x00);
  Frame_Period_Max_Bound_Upper1=SPI_SendReceive(0xff);//接收高位的帧率
  delay_ms(5);
  Frame_Period_Max_Bound_Lower1=SPI_SendReceive(Frame_Period_Lower+0x00);
  Frame_Period_Max_Bound_Lower1=SPI_SendReceive(0xff); //接收低位的帧率
  OFF_CS();
  return ((Frame_Period_Max_Bound_Upper1 << 8) | Frame_Period_Max_Bound_Lower1);
}

void Read_Data_burst(void)
{
	extern u16 Alt_ultrasonic;
  static int SumX;
  static int SumY;

  unsigned char move=0;
  int  x=0;
  int  y=0;
//burst读。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。
ON_CS();
SPI_SendReceive(0x50);   
delay_us(75);
move=SPI_SendReceive(0xFF);             
x=SPI_SendReceive(0xFF);
y=SPI_SendReceive(0xFF);
	if(x&0x80)
	  {
	  //x的二补码转换	
	  x -= 1;
	  x = ~x;	
	  x=(-1)*x;
	  x-=256;
 	  }
	if(y&0x80)
	  {
	  //y的二补码转换	
	  y -= 1;
	  y = ~y;	
	  y=(-1)*y;
	  y-=256;
	  } 
	SumX=SumX+x;             //累加X读入的移动数据
	SumY=SumY+y;			 //累加Y读入的移动数据
	OFF_CS();
	delay_us(4);
	OFF_CS();
sum_x=(25.4*(float)SumX *Alt_ultrasonic)/(12*1600);//距离=d_x*(25.4/1600)*n   其中n=像高:物高=8毫米:物长
sum_y=(25.4*(float)SumY *Alt_ultrasonic)/(12*1600);
  if(move&0x10!=1)
	if(move&0x80)
	{
	  Sys_Printf(Printf_USART,"%d,%d\n",sum_x,sum_y);
	}
		else
		{
			x=0;
			y=0;
		}
x=0;
y=0;
}

float  read_average_pixel(void)	  //读平均像素
{
  float temp;
  ON_CS();
  temp=SPI_SendReceive(Pixel_Sum);
  delay_us(76);
  temp=SPI_SendReceive(0xff);
  temp=temp*256/900;
  OFF_CS();
  return temp;
}

void read_pixel(void)
{
   u8 i,j ,regValue, pixelValue,test=1;	 
  writr_register(Frame_Capture,0x83); 
  delay_us(1010);//等待3帧 (1/3000)*1000000*3+10 =1010us
  //显示数据  30*30=900
  for(i=0;i<30;i++)//列
  {
	  for(j=0;j<30;j++) //行 
	  {
	   regValue=read_register(Frame_Capture);  //读像素
	    if( test && ((regValue&0x40)==0)) //找不到第一个像素
		{
//	 Sys_Printf(Printf_USART,"Read pixel fail");		    
		}
						    test=0;
	                        pixelValue =(regValue<<2);
							while(!(USART1->SR&(1<<6)));
	  						USART1->DR=pixelValue;                               
	                        delay_us(50);
	  }
   }
  ADNS3080_reset();//重启运行
}

void read_pixel_burst(void)//爆发读图像
{
int i,j;
writr_register(Frame_Capture,0x83); 
delay_us(1010);//等待3帧 (1/3000)*1000000*3+10 =1010us
//开始burst读
ON_CS();
SPI_SendReceive(0x40);   
delay_us(75);
for(i=0;i<30;i++)
{
	for(j=0;j<30;j++)
	{
	while(!(USART1->SR&(1<<6)));
	USART1->DR=(SPI_SendReceive(0xFF)<<2); 
    //delay_us(10);
	}
} 
OFF_CS();
delay_us(4);
OFF_CS();            
}

 void GPIO_Config(void)
  {
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA|RCC_AHB1Periph_GPIOB|RCC_AHB1Periph_GPIOC|RCC_AHB1Periph_GPIOD,ENABLE);//	时钟全打开了	
    GPIO_InitTypeDef GPIO_InitStructure;              //定义一个结构体

		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;      //复用推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      //推挽复用输出
	  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        //上拉
		GPIO_Init(GPIOC, &GPIO_InitStructure);
		
//    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10;
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;      //复用推挽输出
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      //推挽复用输出
//	  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        //上拉
//    GPIO_Init(GPIOA, &GPIO_InitStructure);
  }
void  ADNS_Init(void)
{
	GPIO_Config();
 ADNS3080_Init();
  delay_ms(10); 
}