#include <reg52.h>
#include <intrins.h>
#include <stdio.h>

// 各种传感器
sbit STUMBLE = P1^0;   // 跌倒传感器
sbit DHT_IO  = P1^1;   // DHT11 Data
sbit ADC_CS  = P1^2;   // 模数转换 CS
sbit ADC_CK  = P1^3;   // 模数转换 CLK
sbit ADC_IO  = P1^4;   // 模数转换 DI&DO

// LCD
sbit LCD_RS  = P2^7;   // LCD RS
sbit LCD_RW  = P2^6;   // LCD RW
sbit LCD_EN  = P2^5;   // LCD EN

// 按钮
sbit Key_1   = P3^0;   // 设置
sbit Key_2   = P3^1;   // 加
sbit Key_3   = P3^2;   // 减
sbit Key_4   = P3^3;   // 开关

// 输出
sbit BUZZER  = P2^0;   // 蜂鸣器
sbit LED1    = P2^1;   // 跌倒警示灯
sbit LED2    = P2^2;   // 温度警示灯
sbit LED3    = P2^3;   // 瓦斯警示灯

// 系统开关
unsigned char flag = 0;

// 阈值 {瓦斯, 温度}
int threshold[2] = {50, 30};

// LCD 字符串缓存
unsigned char LCDStr[16];


// 初始化各种变量
int gas = 0;
int temp = 27;
int rh = 38;

unsigned char rec_dat[13];

// 延时
void delay(unsigned int ms)
{
	unsigned int i, j;
	for(i = ms; i > 0; i--)
		for(j = 115; j > 0; j--);
}

void delay_us(unsigned char n)
{
    while(--n);
}

// LCD 写命令
void LCDWriteCmd(unsigned char cmd)
{
	LCD_RS = 0;
	LCD_RW = 0;
	LCD_EN = 0;
	P0 = cmd;
	delay(2);
	LCD_EN = 1;
	delay(2);
	LCD_EN = 0;
}

// LCD 写数据
void LCDWriteData(unsigned char dat)
{
	LCD_RS = 1;
	LCD_RW = 0;
	LCD_EN = 0;
	P0 = dat;
	delay(2);
	LCD_EN = 1;
	delay(2);
	LCD_EN = 0;
}

// LCD 初始化
void LCDInit()
{
	LCDWriteCmd(0x38);   // 16*2显示，5*7点阵，8位数据口
	LCDWriteCmd(0x0C);   // 不显示光标
	LCDWriteCmd(0x06);   // 当写入数据后光标自动右移
	LCDWriteCmd(0x01);   // 清屏
}

// LCD 调整光标位置：x-列，y-行
void LCDSetCursor(unsigned char x, unsigned char y)
{
	if(y == 0)
		LCDWriteCmd(0x80 + x);
	else if(y == 1)
		LCDWriteCmd(0x80 + 0x40 + x);
}

// LCD 显示字符串
void LCDPrintStr(unsigned char *str)
{
	while(*str != '\0')
		LCDWriteData(*str++);
}

// 获取 ADC 数据
void GetADC()
{
	unsigned char i;
	unsigned char dat1 = 0;
	unsigned char dat2 = 0;
	ADC_CK = 0;
	ADC_IO = 1;
	ADC_CS = 0;
	ADC_CK = 1;
	ADC_CK = 0;
	ADC_IO = 1;
	ADC_CK = 1;
	ADC_CK = 0;
	ADC_IO = 0;
	ADC_CK = 1;
	ADC_CK = 0;
	ADC_IO = 1;
	for(i=0; i<8; i++)			// 第一次读取
	{
		dat1 <<= 1;
		ADC_CK = 1;
		ADC_CK = 0;
		if(ADC_IO)
			dat1 = dat1 | 0x01;
		else
			dat1 = dat1 | 0x00;
	}
	for(i=0; i<8; i++)			// 第二次读取
	{
		dat2 >>= 1;
		if(ADC_IO)
			dat2 = dat2 | 0x80;
		else
			dat2 = dat2 | 0x00;
		ADC_CK = 1;
		ADC_CK = 0;
	}
	// 结束此次传输
	ADC_IO = 1;
	ADC_CK = 1;
	ADC_CS = 1;

	if(dat1 == dat2)			// 返回采集结果
		gas = dat1;
	else
		gas = 0;
	
	gas = gas / 3;
	if(gas > 100)
		gas = 100;
}

void DHT11Start()
{
	DHT_IO=1;
  delay_us(2);
  DHT_IO=0;
	delay(25);   //拉低延时18ms以上
  DHT_IO=1;
  delay_us(30);   //拉高 延时 20~40us，取中间值 30us
}

// 读取 1 Byte
unsigned char DHT11RecByte()
{
  unsigned char i, dat = 0;
	for(i=0;i<8;i++)
	{
		while(!DHT_IO);
    delay_us(8);
    dat <<= 1;
    if(DHT_IO == 1)
			dat += 1;
		while(DHT_IO);
	}
  return dat;
}
// DHT11 数据
void GetDHT11()
{
	unsigned char R_H, R_L, T_H, T_L, RH, RL, TH, TL, revise;
  DHT11Start();
  if(DHT_IO == 0)
  {
		while(DHT_IO == 0);
		delay_us(40);
    R_H = DHT11RecByte();
    R_L = DHT11RecByte();
    T_H = DHT11RecByte();
    T_L = DHT11RecByte();
    revise = DHT11RecByte();
    delay_us(25);
    if((R_H + R_L + T_H + T_L) == revise)   // 校验
    {
			RH = R_H;
      RL = R_L;
      TH = T_H;
      TL = T_L;
    }
		rh = RH;     // 湿度
		temp = TH;   // 温度
	}
}

void main()
{
	// 初始化 LCD
	LCDInit();
	
	LCDSetCursor(0, 0);
	sprintf(LCDStr, "   Welcome to   ");
	LCDPrintStr(LCDStr);
	sprintf(LCDStr, "      SBAS      ");
	LCDSetCursor(0, 1);
	LCDPrintStr(LCDStr);
	
	delay(1000);
	
	LCDSetCursor(0, 0);
	sprintf(LCDStr, "                ");
	LCDPrintStr(LCDStr);
	LCDSetCursor(0, 1);
	sprintf(LCDStr, "                ");
	LCDPrintStr(LCDStr);
	
	while(1)
	{
		GetDHT11();
		GetADC();
		LCDSetCursor(0, 0);
		sprintf(LCDStr, "T:%2d  R:%2d  G:%2d", temp, rh, gas);
		LCDPrintStr(LCDStr);
		LCDSetCursor(0, 1);
		if(flag == 1)
			sprintf(LCDStr, "* ON");
		else
			sprintf(LCDStr, "*OFF");
		LCDPrintStr(LCDStr);
	}
}