#include <reg52.h>
#include <intrins.h>

// 各种传感器
sbit STUMBLE = P1^0;   // 跌倒传感器
sbit DS18B20 = P1^1;   // 温度传感器
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

// 延时
void delay(unsigned int ms)
{
	unsigned int i, j;
	for(i = ms; i > 0; i--)
		for(j = 115; j > 0; j--);
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
	LCDWriteCmd(0x06);   // 地址+1，当写入数据后光标右移
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

// LCD 显示数字
void LCDPrintNum(unsigned char num)
{
	LCDWriteData(num / 100 + 48);        // 百位
	LCDWriteData(num % 100 / 10 + 48);   // 十位
	LCDWriteData(num % 10 + 48);         // 个位
}

void main()
{
	int gas = 0;
	int temp = 0;
	int rh = 0;
	
	// 初始化
	LCDInit();
	
	while(1)
	{
		LCDSetCursor(0, 0);
		LCDPrintStr("Test MSG");
	}
}