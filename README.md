# yushi
这是一个写着玩的单片机课程作业，基于 C51，这个分支用于制作实物

## 功能说明
1. 可通过 DHT11 获得温湿度并显示在 LCD 屏上
2. 可通过燃气传感器（仿真使用滑动变阻器+ADC0832代替）获得燃气浓度并显示在 LCD 屏上
3. 可通过传感器（仿真使用按钮代替）检测老人摔倒
4. 可调整各参数阈值，超过阈值声光报警或启动电机

## 文件说明
- `Project.pdsprj` Proteus仿真
- `Project.uvproj` Keil工程
- `main.c` 程序源文件
- `Objects/Project.hex` 编译得到的HEX文件

## 实物照片
> [!TIP]
> 尚未完成，仅为阶段性成果

![wx_20240506220819](https://github.com/klxf/yushi/assets/31070597/8098b385-8195-415d-baea-bea0d30a9d83)


## 开源协议
Apache License 2.0
