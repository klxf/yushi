#include <reg52.h>
#include <intrins.h>
#include <stdio.h>

// 各种传感器
sbit TUMBLE  = P1^0;   // 跌倒传感器
sbit DHT_IO  = P1^1;   // DHT11 Data
sbit ADC_CS  = P1^2;   // 模数转换 CS
sbit ADC_CK  = P1^3;   // 模数转换 CLK
sbit ADC_IO  = P1^4;   // 模数转换 DI&DO

// LCD
sbit LCD_RS  = P3^7;   // LCD RS
sbit LCD_RW  = P3^6;   // LCD RW
sbit LCD_EN  = P3^5;   // LCD EN

// 按钮
sbit Key_1   = P3^0;   // 设置
sbit Key_2   = P3^1;   // 加
sbit Key_3   = P3^2;   // 减
sbit Key_4   = P3^3;   // 开关

// 输出
sbit BUZZER  = P2^0;   // 蜂鸣器
sbit LED1    = P2^1;   // 跌倒警示灯
sbit LED2    = P2^2;   // 温度警示灯
sbit LED3    = P2^3;   // 湿度警示灯
sbit LED4    = P2^4;   // 瓦斯警示灯
sbit MOTOR   = P2^5;   // 风扇马达

// 系统开关变量
bit flag      = 0;   // 总开关
bit isSetting = 0;   // 设置页面

// 阈值 {温度, 湿度, 瓦斯}
int threshold[3] = {40, 60, 5};

// LCD 字符串缓存
unsigned char LCDStr[17];
unsigned char tipStr[7];

// 初始化各种变量
bit isTumble = 0;
int temp = 27;
int rh = 38;
int gas = 0;
unsigned char setting = 0;

unsigned char rec_dat[13];

// 延时
void delay_ms(unsigned int ms)
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
	delay_ms(2);
	LCD_EN = 1;
	delay_ms(2);
	LCD_EN = 0;
}

// LCD 写数据
void LCDWriteData(unsigned char dat)
{
	LCD_RS = 1;
	LCD_RW = 0;
	LCD_EN = 0;
	P0 = dat;
	delay_ms(2);
	LCD_EN = 1;
	delay_ms(2);
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

// 老人摔倒检测
void GetTumble()
{
	if(TUMBLE == 0)
		isTumble = !isTumble;
	while(!TUMBLE);
}

// ADC 启动
void ADCStart()
{
	ADC_CK = 0;   // 电平初始化
	ADC_IO = 1;
	delay_us(1);
	ADC_CS = 0;
	ADC_CK = 1;   // 起始信号
	delay_us(1);
	ADC_CK = 0;
	ADC_IO = 1;
	ADC_CK = 1;   // 通道选择第一位
	delay_us(1);
	ADC_CK = 0;
	ADC_IO = 0;
	ADC_CK = 1;   // 通道选择第二位
	delay_us(1);
	ADC_CK = 0;
	ADC_IO = 1;
}

// 获取 ADC 数据
void GetADC()
{
	unsigned char i;
	unsigned char dat1 = 0, dat2 = 0;
	ADCStart();
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

	if(dat1 == dat2)			// 校验采集结果
		gas = dat1;
	else
		gas = 0;
	
	gas = gas / 3;
	if(gas > 100)
		gas = 100;
}

// DHT11 启动
void DHT11Start()
{
	DHT_IO=1;
	delay_us(2);
	DHT_IO=0;
	delay_ms(25);
	DHT_IO=1;
	delay_us(30);
}

// 读取 DHT11 1 Byte 数据
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

// 获取 DHT11 数据
void GetDHT11()
{
	unsigned char R_H, R_L, T_H, T_L, RH, RL, TH, TL, revise;
	DHT11Start();
	if(DHT_IO == 0)
	{
		while(DHT_IO == 0);
		delay_us(40);
		R_H = DHT11RecByte();      // 湿度高四位
		R_L = DHT11RecByte();      // 湿度低四位
		T_H = DHT11RecByte();      // 湿度高四位
		T_L = DHT11RecByte();      // 湿度低四位
		revise = DHT11RecByte();   // 校验位
		delay_us(25);
		if((R_H + R_L + T_H + T_L) == revise)   // 对数据进行校验
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

// 按键事件
void KeyEvents()
{
	if(Key_1 == 0)   // 设置按钮
	{
		isSetting = 1;
		setting++;
		
		// 显示设置 UI
		LCDWriteCmd(0x0C);
		LCDSetCursor(0, 0);
		sprintf(LCDStr, "    SETTINGS    ");
		LCDPrintStr(LCDStr);
		LCDSetCursor(0, 1);
		sprintf(LCDStr, "T:%2d  R:%2d  G:%2d", threshold[0], threshold[1], threshold[2]);
		LCDPrintStr(LCDStr);
		
		// 切换光标位置
		LCDWriteCmd(0x0F);
		if(setting == 1)
		{
			LCDSetCursor(0, 1);
		}
		else if(setting == 2)
		{
			LCDSetCursor(6, 1);
		}
		else if(setting == 3)
		{
			LCDSetCursor(12, 1);
		}
		else
		{
			setting = 0;
			isSetting = 0;
			LCDWriteCmd(0x0C);
		}
		while(!Key_1);
	}
	else if(Key_2 == 0)   // 增加按钮
	{
		if(isSetting == 0)
			return;
		if(setting == 1)
		{
			threshold[0]++;
			LCDSetCursor(2, 1);
			sprintf(LCDStr, "%2d", threshold[0]);
			LCDPrintStr(LCDStr);
			
			LCDSetCursor(0, 1);
		}
		else if(setting == 2)
		{
			threshold[1]++;
			LCDSetCursor(8, 1);
			sprintf(LCDStr, "%2d", threshold[1]);
			LCDPrintStr(LCDStr);
			
			LCDSetCursor(6, 1);
		}
		else if(setting == 3)
		{
			threshold[2]++;
			LCDSetCursor(14, 1);
			sprintf(LCDStr, "%2d", threshold[2]);
			LCDPrintStr(LCDStr);
			
			LCDSetCursor(12, 1);
		}
		while(!Key_2);
	}
	else if(Key_3 == 0)   // 减小按钮
	{
		if(isSetting == 0)
			return;
		if(setting == 1)
		{
			threshold[0]--;
			LCDSetCursor(2, 1);
			sprintf(LCDStr, "%2d", threshold[0]);
			LCDPrintStr(LCDStr);
			
			LCDSetCursor(0, 1);
		}
		else if(setting == 2)
		{
			threshold[1]--;
			LCDSetCursor(8, 1);
			sprintf(LCDStr, "%2d", threshold[1]);
			LCDPrintStr(LCDStr);
			
			LCDSetCursor(6, 1);
		}
		else if(setting == 3)
		{
			threshold[2]--;
			LCDSetCursor(14, 1);
			sprintf(LCDStr, "%2d", threshold[2]);
			LCDPrintStr(LCDStr);
			
			LCDSetCursor(12, 1);
		}
		while(!Key_3);
	}
	else if(Key_4 == 0)   // 开关按钮
	{
		flag = !flag;
		while(!Key_4);
	}
}

// 判断
void Check()
{
	sprintf(tipStr, "      ");
	if(isTumble == 1)
	{
		LED1 = 0;
		BUZZER = 0;
		sprintf(tipStr, "TUMBLE");
		return;
	}
	else
	{
		LED1 = 1;
		BUZZER = 1;
	}
	
	if(temp >= threshold[0])
	{
		LED2 = 0;
		BUZZER = 0;
		sprintf(tipStr, "  FIRE");
		return;
	}
	else
	{
		LED2 = 1;
		BUZZER = 1;
	}
	
	if(rh >= threshold[1])
	{
		LED3 = 0;
		MOTOR = 0;
		sprintf(tipStr, " MOTOR");
		return;
	}
	else
	{
		LED3 = 1;
		MOTOR = 1;
	}
	
	if(gas >= threshold[2])
	{
		LED4 = 0;
		BUZZER = 0;
		sprintf(tipStr, "   GAS");
		return;
	}
	else
	{
		LED4 = 1;
		BUZZER = 1;
	}
}

void main()
{
	// 初始化 LCD
	LCDInit();
	
	LCDSetCursor(0, 0);
	sprintf(LCDStr, "   Welcome to   ");
	LCDPrintStr(LCDStr);
	LCDSetCursor(0, 1);
	sprintf(LCDStr, "      SBAS      ");
	LCDPrintStr(LCDStr);
	
	delay_ms(1000);
	sprintf(tipStr, "      ");
	
	LCDSetCursor(0, 0);
	sprintf(LCDStr, "                ");
	LCDPrintStr(LCDStr);
	LCDSetCursor(0, 1);
	sprintf(LCDStr, "                ");
	LCDPrintStr(LCDStr);
	
	while(1)
	{
		GetTumble();  // 获取老人是否摔倒
		GetDHT11();   // 获取温湿度数据
		GetADC();     // 获取ADC数据
		KeyEvents();  // 按键事件
		
		if(flag)
			Check();    // 确认各值
		
		if(isSetting == 0){
			LCDSetCursor(0, 0);
			sprintf(LCDStr, "T:%2d  R:%2d  G:%2d", temp, rh, gas);
			LCDPrintStr(LCDStr);
			LCDSetCursor(0, 1);
			if(flag)
				sprintf(LCDStr, "* ON      %s", tipStr);
			else
				sprintf(LCDStr, "*OFF            ");
			LCDPrintStr(LCDStr);
		}
		
	}
}