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
bit isDebug   = 0;   // Debug 页面

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
unsigned char DHT11Data, DHT11Flag;

// 毫秒延时
void delay_ms(unsigned int ms)
{
	unsigned int i, j;
	for(i = ms; i > 0; i--)
		for(j = 115; j > 0; j--);
}

// 10微秒延时
void delay_10us(void) {
	unsigned char i;
	i--;
	i--;
	i--;
	i--;
	i--;
	i--;
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

// LCD 调整光标位置：column-列，line-行
void LCDSetCursor(unsigned char column, unsigned char line)
{
	if(line == 0)
		LCDWriteCmd(0x80 + column);
	else if(line == 1)
		LCDWriteCmd(0x80 + 0x40 + column);
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
	delay_10us();
	ADC_CS = 0;
	ADC_CK = 1;   // 起始信号
	delay_10us();
	ADC_CK = 0;
	ADC_IO = 1;
	ADC_CK = 1;   // 通道选择第一位
	delay_10us();
	ADC_CK = 0;
	ADC_IO = 0;
	ADC_CK = 1;   // 通道选择第二位
	delay_10us();
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
	
	if(!isDebug)
	{
		gas = gas / 100;    // 此处转换方式并不准确，仅作测试
		if(gas > 100)
			gas = 100;
	}
}

// 读取DHT11数据
void ReadDHT11(void) {
	unsigned char i, tempdata;
	
	for (i = 0; i < 8; i++) {
		DHT11Flag = 2;
		while ((!DHT_IO) && DHT11Flag++);
		delay_10us();
		delay_10us();
		delay_10us();
		tempdata = 0;
		if (DHT_IO) tempdata = 1;
		DHT11Flag = 2;
		while ((DHT_IO) && DHT11Flag++);
		if (DHT11Flag == 1) break;
		
		DHT11Data <<= 1;
		DHT11Data |= tempdata;
	}
}

// 获取 DHT11 数据
void GetDHT11(void) {
	unsigned char RH_data_H_temp, RH_data_L_temp, TP_data_H_temp, TP_data_L_temp, checkdata_temp;
	unsigned char checkdata;
	// 主机拉低18ms
	DHT_IO = 0;
	delay_ms(18);
	DHT_IO = 1;
	// 总线由上拉电阻拉高，主机延时20us
	delay_10us();
	delay_10us();
	// 主机设为输入，判断从机响应信号
	DHT_IO = 1;

	if (!DHT_IO) { // T !
		DHT11Flag = 2;
		// 判断从机是否发出 80us 的低电平响应信号是否结束
		while ((!DHT_IO) && DHT11Flag++);
		DHT11Flag = 2;
		// 判断从机是否发出 80us 的高电平，如发出则进入数据接收状态
		while ((DHT_IO) && DHT11Flag++);

		ReadDHT11();
		RH_data_H_temp = DHT11Data;
		ReadDHT11();
		RH_data_L_temp = DHT11Data;
		ReadDHT11();
		TP_data_H_temp = DHT11Data;
		ReadDHT11();
		TP_data_L_temp = DHT11Data;
		ReadDHT11();
		checkdata_temp = DHT11Data;

		DHT_IO = 1;

		// 数据校验
		checkdata = (RH_data_H_temp + RH_data_L_temp + TP_data_H_temp + TP_data_L_temp);
		if (checkdata == checkdata_temp) {
			rh = RH_data_H_temp;
			temp = TP_data_H_temp;
		}
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
	else if(Key_2 == 0)          // 增加按钮
	{
		if(isSetting == 0)         // 若不处于设置模式则 return
			return;
		if(setting == 1)           // 加温度
		{
			threshold[0]++;
			LCDSetCursor(2, 1);
			sprintf(LCDStr, "%2d", threshold[0]);
			LCDPrintStr(LCDStr);
			
			LCDSetCursor(0, 1);
		}
		else if(setting == 2)      // 加湿度
		{
			threshold[1]++;
			LCDSetCursor(8, 1);
			sprintf(LCDStr, "%2d", threshold[1]);
			LCDPrintStr(LCDStr);
			
			LCDSetCursor(6, 1);
		}
		else if(setting == 3)      // 加瓦斯
		{
			threshold[2]++;
			LCDSetCursor(14, 1);
			sprintf(LCDStr, "%2d", threshold[2]);
			LCDPrintStr(LCDStr);
			
			LCDSetCursor(12, 1);
		}
		while(!Key_2);
	}
	else if(Key_3 == 0)          // 减小按钮
	{
		if(isSetting == 0)         // 若不处于设置模式则 return
			return;
		if(setting == 1)           // 减温度
		{
			threshold[0]--;
			LCDSetCursor(2, 1);
			sprintf(LCDStr, "%2d", threshold[0]);
			LCDPrintStr(LCDStr);
			
			LCDSetCursor(0, 1);
		}
		else if(setting == 2)      // 减湿度
		{
			threshold[1]--;
			LCDSetCursor(8, 1);
			sprintf(LCDStr, "%2d", threshold[1]);
			LCDPrintStr(LCDStr);
			
			LCDSetCursor(6, 1);
		}
		else if(setting == 3)      // 减瓦斯
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
		if(isSetting)
		{
			isDebug = !isDebug;
			// 清屏
			LCDSetCursor(0, 0);
			sprintf(LCDStr, "           DeBug");
			LCDPrintStr(LCDStr);
			LCDSetCursor(0, 1);
			sprintf(LCDStr, "                ");
			LCDPrintStr(LCDStr);
		}
		else
			flag = !flag;
		
		while(!Key_4);
	}
}

// 判断
void Check()
{
	sprintf(tipStr, "      ");
	// 摔倒判断
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
	// 温度判断
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
	// 湿度判断
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
	// 瓦斯判断
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
	
	// 欢迎字幕
	LCDSetCursor(0, 0);
	sprintf(LCDStr, "   Welcome to   ");
	LCDPrintStr(LCDStr);
	LCDSetCursor(0, 1);
	sprintf(LCDStr, "      SBAS      ");
	LCDPrintStr(LCDStr);
	
	delay_ms(1000);
	
	sprintf(tipStr, "      ");
	
	// 清屏
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
		
		if((isSetting || isDebug) == 0)
		{
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
		if(isDebug)
		{
			LCDSetCursor(0, 0);
			sprintf(LCDStr, "G:%d", gas);
			LCDPrintStr(LCDStr);
			LCDSetCursor(0, 1);
			sprintf(LCDStr, "T:%2d  R:%2d", temp, rh);
			LCDPrintStr(LCDStr);
		}
	}
}
