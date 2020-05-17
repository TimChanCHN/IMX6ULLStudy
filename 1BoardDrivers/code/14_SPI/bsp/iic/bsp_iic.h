#ifndef _BSP_I2C_H
#define _BSP_I2C_H

#include "imx6ul.h"
/* 本例程是硬件IIC例程 */

/* 对于IIC写，其流程是先写从设备地址(bit0为0,写操作)，再写要操作的寄存器范围，最后写数据 */
/* 对于IIC读，其流程是先写从设备地址(bit0为0,写操作)，再写要操作的寄存器范围，再写一次从设备地址(bit0为1,读操作)，最后才开始读数据 */

/* 相关宏定义 */
#define I2C_STATUS_OK				(0)
#define I2C_STATUS_BUSY				(1)
#define I2C_STATUS_IDLE				(2)
#define I2C_STATUS_NAK				(3)
#define I2C_STATUS_ARBITRATIONLOST	(4)
#define I2C_STATUS_TIMEOUT			(5)
#define I2C_STATUS_ADDRNAK			(6)

/* I2C方向 */
enum i2c_direction
{
    kI2C_Write = 0x0, 		/* 主机向从机写数据 */
    kI2C_Read = 0x1,  		/* 主机从从机读数据 */
};

/* 主机传输结构体 */
struct i2c_transfer
{
    unsigned char slaveAddress;      	/* 7位从机地址 			*/
    enum i2c_direction direction; 		/* 传输方向 			*/
    unsigned int subaddress;       		/* 寄存器地址			*/
    unsigned char subaddressSize;    	/* 寄存器地址长度 			*/
    unsigned char *volatile data;    	/* 数据缓冲区 			*/
    volatile unsigned int dataSize;  	/* 数据缓冲区长度 			*/
};

/* 函数声明 */
void i2c_init(I2C_Type *base);
unsigned char i2c_master_start(I2C_Type *base, unsigned char address, enum i2c_direction direction);
unsigned char i2c_master_repeated_start(I2C_Type *base, unsigned char address,  enum i2c_direction direction);
unsigned char i2c_check_and_clear_error(I2C_Type *base, unsigned int status);
unsigned char i2c_master_stop(I2C_Type *base);
void i2c_master_write(I2C_Type *base, const unsigned char *buf, unsigned int size);
void i2c_master_read(I2C_Type *base, unsigned char *buf, unsigned int size);
unsigned char i2c_master_transfer(I2C_Type *base, struct i2c_transfer *xfer);



#endif
