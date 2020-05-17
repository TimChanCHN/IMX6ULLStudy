# 8.SPI
## 1. SPI原理
1. SPI是一种高速、全双工、同步的串行通讯总线。它拥有四根线：CS, SCLK, MOSI, MISO。通过片选信号CS进行寻址，主机通过MOSI输出，MISO输入，从机反之。
2. SPI有四种不同的通讯模式，由CPOL(时钟极性)和CPHA(时钟相位)选择通讯模式
   1. MODE0: CPOL=0,CPHA=0--当SCLK=1时为有效态，数据采样在第1个边沿，数据发送在第2个边沿
   2. MODE1: CPOL=0,CPHA=1--当SCLK=1时为有效态，数据采样在第2个边沿，数据发送在第1个边沿
   3. MODE2: CPOL=1,CPHA=0--当SCLK=0时为有效态，数据采样在第1个边沿，数据发送在第2个边沿
   4. MODE3: CPOL=1,CPHA=1--当SCLK=0时为有效态，数据采样在第2个边沿，数据发送在第1个边沿
   ![SPI时序图](https://github.com/TimChanCHN/pictures/raw/master/imx6ul/SPI.png)
3. SPI每次数据传输，都会通过移位寄存器，完成一个字节数据的传输。
4. 和IIC相比，SPI传输速度更快，并且支持全双工通讯，但是没有应答机制确认是否收到数据，因此不比IIC可靠
   
## 2. IMX6ULL设置
1. RXDATA寄存器：数据接收寄存器
2. TXDATA寄存器：数据发送寄存器
3. CONREG寄存器：
   1. bit0:使能SPI
   2. bit3:向TXFIFO写入数据后马上开启SPI突发访问，也就是发送数据
   3. bit7:4 :选择SPI通道主从模式，bit7~4分别是通道3~0
   4. bit19:18:设置对应的通道
   5. bit31:30:设置突发访问长度
4. CONFIGREG寄存器
   1. bit0:PHA
   2. bit4:PO
   3. bit12:0
   4. bit16:空闲时数据线状态
   5. bit20:
5. STATREG寄存器
   1. bit0：表示TXFIFO是否为空
   2. bit3: 表示RXFIFO是否有数据
6. PERIODREG寄存器
   1. bit14:0 : 设置wait states时间
   2. Bit15 : wait states的时钟源为SPI CLK
   3. Bit21:16 : 片选信号的延时
    
