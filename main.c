#include <reg52.h>
#include <intrins.h>

// 各种传感器
sbit STUMBLE = P1^0;   // 跌倒传感器
sbit DS18B20 = P1^1;   // 温度传感器
sbit ADC_CS  = P1^2;   // 模数转换 CS
sbit ADC_CK  = P1^3;   // 模数转换 CLK
sbit ADC_IO  = P1^4;   // 模数转换 DI&DO

// LCD
sbit LCD_PS  = P2^7;   // LCD RS
sbit LCD_RW  = P2^8;   // LCD RW
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

void main()
{
	int gas = 0;   // 瓦斯
	int tem = 0;   // 温度
}