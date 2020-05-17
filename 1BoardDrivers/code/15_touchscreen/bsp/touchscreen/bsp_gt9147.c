#include "bsp_gt9147.h"
#include "bsp_iic.h"
#include "bsp_int.h"
#include "bsp_delay.h"
#include "stdio.h"

/*
 * GT9147配置参数表
 * 第一个字节为版本号(0X61),必须保证新的版本号大于等于GT9147内部
 * flash原有版本号,才会更新配置.
 */
u8 GT9147_CFG_TBL[]=
{ 
	0x41,0xe0,0x01,0x10,0x01,0x05,0x0d,0x00,0x01,0x08,
	0x28,0x05,0x50,0x32,0x03,0x05,0x00,0x00,0xff,0xff,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x89,0x28,0x0a,
	0x17,0x15,0x31,0x0d,0x00,0x00,0x02,0x9b,0x03,0x25,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x32,0x00,0x00,
	0x00,0x0f,0x94,0x94,0xc5,0x02,0x07,0x00,0x00,0x04,
	0x8d,0x13,0x00,0x5c,0x1e,0x00,0x3c,0x30,0x00,0x29,
	0x4c,0x00,0x1e,0x78,0x00,0x1e,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x08,0x0a,0x0c,0x0e,0x10,0x12,0x14,0x16,
	0x18,0x1a,0x00,0x00,0x00,0x00,0x1f,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0x00,0x02,0x04,0x05,0x06,0x08,0x0a,0x0c,
	0x0e,0x1d,0x1e,0x1f,0x20,0x22,0x24,0x28,0x29,0xff,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,
};

struct gt9147_dev_struc gt9147_dev;

/* 初始化触摸屏，即初始化触摸屏控制芯片GT9147 */
void gt9147_init(void)
{
    unsigned char temp[5]; 
    unsigned char regvalue;
	gpio_pin_config_t ctintpin_config;
	gpio_pin_config_t ctretpin_config;
	gt9147_dev.initfalg = GT9147_INIT_NOTFINISHED;

	/* 1、初始化IIC2 IO
     * I2C2_SCL -> UART5_TXD
     * I2C2_SDA -> UART5_RXD
     */
	IOMUXC_SetPinMux(IOMUXC_UART5_TX_DATA_I2C2_SCL,1);
	IOMUXC_SetPinMux(IOMUXC_UART5_RX_DATA_I2C2_SDA,1);

	/* 配置I2C2 IO属性	
	 *bit 16:0 HYS关闭
	 *bit [15:14]: 1 默认47K上拉
	 *bit [13]: 1 pull功能
	 *bit [12]: 1 pull/keeper使能 
	 *bit [11]: 0 关闭开路输出
	 *bit [7:6]: 10 速度100Mhz
	 *bit [5:3]: 110 驱动能力为R0/6
	 *bit [0]: 1 高转换率
	 */
	IOMUXC_SetPinConfig(IOMUXC_UART5_TX_DATA_I2C2_SCL, 0x70B0);
	IOMUXC_SetPinConfig(IOMUXC_UART5_RX_DATA_I2C2_SDA, 0X70B0);
	
	/* 2、初始化触摸屏中断IO和复位IO */
	IOMUXC_SetPinMux(IOMUXC_GPIO1_IO09_GPIO1_IO09,0);		/* 复用为GPIO1_IO9 */
	IOMUXC_SetPinMux(IOMUXC_SNVS_SNVS_TAMPER9_GPIO5_IO09,0);/* 复用为GPIO5_IO9 */
	
	IOMUXC_SetPinConfig(IOMUXC_GPIO1_IO09_GPIO1_IO09,0x10B0);
	IOMUXC_SetPinConfig(IOMUXC_SNVS_SNVS_TAMPER9_GPIO5_IO09,0X10B0);

	/* 中断IO初始化 */
	ctintpin_config.direction = kGPIO_DigitalOutput;
	ctintpin_config.outputLogic = 1;
	ctintpin_config.interruptMode = kGPIO_NoIntmode;
	gpio_init(GPIO1, 9, &ctintpin_config);

	/* 复位IO初始化 */
    ctretpin_config.direction = kGPIO_DigitalOutput;	
    ctretpin_config.interruptMode = kGPIO_NoIntmode;	
    ctretpin_config.outputLogic = 1;					   
    gpio_init(GPIO5, 9, &ctretpin_config); 

	/* 3、初始化I2C */
	i2c_init(I2C2);	

	/* 4、初始化GT9147，要严格按照GT9147时序要求 */
	gpio_pinwrite(GPIO5, 9, 0);	/* 复位GT9147 */
	delayms(10);
	gpio_pinwrite(GPIO5, 9, 1); /* 停止复位GT9147 */ 
	delayms(10);
	gpio_pinwrite(GPIO1, 9, 0);	/* 拉低INT引脚 */
	delayms(100);

	/* 重新设置中断IO，配置为中断功能 */
	IOMUXC_SetPinConfig(IOMUXC_GPIO1_IO09_GPIO1_IO09,0x0080);
	ctintpin_config.direction = kGPIO_DigitalInput;
	ctintpin_config.interruptMode = kGPIO_IntRisingOrFallingEdge;
	ctintpin_config.outputLogic=0;
	gpio_init(GPIO1, 9, &ctintpin_config);

    gt9147_read_len(GT9147_ADDR, GT_PID_REG, 4, temp);/* 读取产品ID */
    temp[4] = 0;
	printf("CTP ID:%s\r\n", temp);	    /* 打印ID */
	gt9147_write_byte(GT9147_ADDR, GT_CTRL_REG, 0x02);
	regvalue = gt9147_read_byte(GT9147_ADDR, GT_CFGS_REG);
	printf("Default Ver:%#x\r\n",regvalue);
	gt9147_send_cfg(0);/* 配置GT9147 */
	delayms(10); 
	gt9147_write_byte(GT9147_ADDR, GT_CTRL_REG, 0);	

	GIC_EnableIRQ(GPIO1_Combined_0_15_IRQn);				/* 使能GIC中对应的中断 */
	system_register_irqhandler(GPIO1_Combined_0_15_IRQn, (system_irq_handler_t)gt9147_irqhandler, NULL);	/* 注册中断服务函数 */
	gpio_enableint(GPIO1, 9);								/* 使能GPIO1_IO09的中断功能 */
    delayms(110);

    gt9147_dev.initfalg = GT9147_INIT_FINISHED;	/* 标记GT9147初始化完成 */
	gt9147_dev.intflag = 0;
}

/* gt9147中断处理函数 */
void gt9147_irqhandler(void)
{ 
	if(gt9147_dev.initfalg == GT9147_INIT_FINISHED)
	{
		gt9147_dev.intflag = 1;
		//gt9147_write_byte(GT9147_ADDR, GT_GSTID_REG, 0); 	
		gt9147_read_tpcoord();
	}
	gpio_clearintflags(GPIO1, 9); /* 清除中断标志位 */
}

/* 向GT9147写入数据 */
unsigned char gt9147_write_byte(unsigned char addr,unsigned int reg, unsigned char data)
{
    unsigned char status=0;
    unsigned char writedata=data;
    struct i2c_transfer masterXfer;
	
    /* 配置I2C xfer结构体 */
   	masterXfer.slaveAddress = addr; 			/* 设备地址 				*/
    masterXfer.direction = kI2C_Write;			/* 写入数据 				*/
    masterXfer.subaddress = reg;				/* 要写入的寄存器地址 			*/
    masterXfer.subaddressSize = 2;				/* 地址长度一个字节 			*/
    masterXfer.data = &writedata;				/* 要写入的数据 				*/
    masterXfer.dataSize = 1;  					/* 写入数据长度1个字节			*/

    if(i2c_master_transfer(I2C2, &masterXfer))
        status=1;
        
    return status;
}

/* 从GT9147读取一个字节的数据 */
unsigned char gt9147_read_byte(unsigned char addr,unsigned int reg)
{
	unsigned char val=0;
	
	struct i2c_transfer masterXfer;	
	masterXfer.slaveAddress = addr;				/* 设备地址 				*/
    masterXfer.direction = kI2C_Read;			/* 读取数据 				*/
    masterXfer.subaddress = reg;				/* 要读取的寄存器地址 			*/
    masterXfer.subaddressSize = 2;				/* 地址长度一个字节 			*/
    masterXfer.data = &val;						/* 接收数据缓冲区 				*/
    masterXfer.dataSize = 1;					/* 读取数据长度1个字节			*/
	i2c_master_transfer(I2C2, &masterXfer);

	return val;
}

/* 从GT9147读取多个字节的数据 */
void gt9147_read_len(unsigned char addr,unsigned int reg,unsigned int len,unsigned char *buf)
{	
	struct i2c_transfer masterXfer;	
	
	masterXfer.slaveAddress = addr;				/* 设备地址 				*/
    masterXfer.direction = kI2C_Read;			/* 读取数据 				*/
    masterXfer.subaddress = reg;				/* 要读取的寄存器地址 			*/
    masterXfer.subaddressSize = 2;				/* 地址长度一个字节 			*/
    masterXfer.data = buf;						/* 接收数据缓冲区 				*/
    masterXfer.dataSize = len;					/* 读取数据长度1个字节			*/
	i2c_master_transfer(I2C2, &masterXfer);
} 

/* 向GT9147多个寄存器写入数据 */
void gt9147_write_len(unsigned char addr,unsigned int reg,unsigned int len, unsigned char *buf)
{	
	struct i2c_transfer masterXfer;	
	
	masterXfer.slaveAddress = addr;				/* 设备地址         */
    masterXfer.direction = kI2C_Write;			/* 读取数据 	    */
    masterXfer.subaddress = reg;				/* 要读取的寄存器地址 */
    masterXfer.subaddressSize = 2;				/* 地址长度一个字节     */
    masterXfer.data = buf;						/* 接收数据缓冲区 	    */
    masterXfer.dataSize = len;					/* 读取数据长度1个字节  */
	i2c_master_transfer(I2C2, &masterXfer);
} 

/* 发送GT9147配置参数 */
void gt9147_send_cfg(unsigned char mode)
{
	unsigned char buf[2];
	unsigned int i = 0;

	buf[0] = 0;
	buf[1] = mode;	/* 是否写入到GT9147 FLASH?  即是否掉电保存 */
	for(i = 0; i < (sizeof(GT9147_CFG_TBL)); i++) /* 计算校验和 */
        buf[0] += GT9147_CFG_TBL[i];            
    buf[0] = (~buf[0]) + 1;

    /* 发送寄存器配置 */
    gt9147_write_len(GT9147_ADDR, GT_CFGS_REG, sizeof(GT9147_CFG_TBL), GT9147_CFG_TBL);
    gt9147_write_len(GT9147_ADDR, GT_CHECK_REG, 2, buf);/* 写入校验和,配置更新标记 */
} 

const u16 GT9147_TPX_TBL[5]={GT_TP1_REG,GT_TP2_REG,GT_TP3_REG,GT_TP4_REG,GT_TP5_REG};

/* 读取当前所有触摸点的坐标 */
void gt9147_read_tpcoord(void)
{
	u8 buf[4];
	u8 i = 0;
	u8 regvalue = 0;

	regvalue = gt9147_read_byte(GT9147_ADDR, GT_GSTID_REG);
	gt9147_write_byte(GT9147_ADDR, GT_GSTID_REG, 0x00);

	/* 读取5点触摸坐标值 */
	for(i = 0; i < 5; i++)
	{
		gt9147_read_len(GT9147_ADDR, GT9147_TPX_TBL[i], 4, buf);	/* 读取坐标值 */
		gt9147_dev.x[i] = ((u16)buf[1] << 8) + buf[0];
		gt9147_dev.y[i] = (((u16)buf[3] << 8) + buf[2]);				
	} 
	gt9147_dev.point_num = regvalue & 0XF;
}
