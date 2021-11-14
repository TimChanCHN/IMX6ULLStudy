#include "bsp_ft5426.h"
#include "bsp_gpio.h"
#include "bsp_int.h"
#include "bsp_delay.h"
#include "bsp_i2c.h"
#include "stdio.h"

struct ft5426_dev_struc ft5426_dev;

/* 初始化FT5426 */
void ft5426_init(void)
{
    unsigned char reg_value[2];


    /* 1、IO初始化 */
    IOMUXC_SetPinMux(IOMUXC_UART5_TX_DATA_I2C2_SCL, 1); /*复用为I2C2_SCL */
    IOMUXC_SetPinMux(IOMUXC_UART5_RX_DATA_I2C2_SDA, 1); /*复用为I2C2_SDA */

    IOMUXC_SetPinConfig(IOMUXC_UART5_TX_DATA_I2C2_SCL, 0X70b0);
    IOMUXC_SetPinConfig(IOMUXC_UART5_RX_DATA_I2C2_SDA, 0X70b0);

    /* 初始化INT引脚，使能中断 */
    IOMUXC_SetPinMux(IOMUXC_GPIO1_IO09_GPIO1_IO09, 0); /*复用为GPIO1_IO09 */
    IOMUXC_SetPinConfig(IOMUXC_GPIO1_IO09_GPIO1_IO09 , 0XF080);

    gpio_pin_config_t ctint_config;
    ctint_config.direction = kGPIO_DigitalInput;
    ctint_config.interruptMode = kGPIO_IntRisingOrFallingEdge;
    gpio_init(GPIO1, 9, &ctint_config);

    GIC_EnableIRQ(GPIO1_Combined_0_15_IRQn );
    system_register_irqhandler(GPIO1_Combined_0_15_IRQn , (system_irq_handler_t)gpio1_io09_irqhandler, NULL);
    gpio_enableint(GPIO1, 9);

    /* 初始化RST引脚，使能中断 */
    IOMUXC_SetPinMux(IOMUXC_SNVS_SNVS_TAMPER9_GPIO5_IO09, 0); /*复用为GPIO5_IO09 */
    IOMUXC_SetPinConfig(IOMUXC_SNVS_SNVS_TAMPER9_GPIO5_IO09, 0X10b0);

    gpio_pin_config_t ctrst_config;
    ctrst_config.direction = kGPIO_DigitalOutput;
    ctrst_config.outputLogic = 1;
    gpio_init(GPIO5, 9, &ctrst_config);

    gpio_pinwrite(GPIO5, 9, 0); /* 复位FT5426 */
    delay_ms(50);
    gpio_pinwrite(GPIO5, 9, 1); /* 停止复位 */
    delay_ms(50);

    /* 2、I2C2接口初始化 */
    i2c_init(I2C2);

    /* 3、FT5426初始化*/
    ft5426_read_len(FT5426_ADDR, FT5426_IDGLIB_VERSION, 2, reg_value);
    printf("Touch Frimware Version:%#X\r\n", 
         ((unsigned short)reg_value[0] << 8) | reg_value[1]);

    ft5426_write_byte(FT5426_ADDR, FT5426_DEVICE_MODE, 0); /* 设置FT5426工作在正常模式 */
    ft5426_write_byte(FT5426_ADDR, FT5426_IDG_MODE, 1); /* 中断模式 */

    ft5426_dev.initflag = FT5426_INIT_FINISHED;
}

/* 中断处理函数 */
void gpio1_io09_irqhandler(unsigned int gicciar, void *param)
{
    if(ft5426_dev.initflag == FT5426_INIT_FINISHED) {    
        ft5426_read_tpcoord();
    }

    /* 清除中断表示位 */
    gpio_clearintflags(GPIO1, 9);
}


/* 向FT5426写一个字节的数据 */
unsigned char ft5426_write_byte(unsigned char addr, unsigned char reg, 
                                    unsigned char data)
{
    unsigned char writedata  = data;
    unsigned char status = 0;
    struct i2c_transfer masterXfer;

    masterXfer.slaveAddress = addr;
    masterXfer.direction = kI2C_Write;
    masterXfer.subaddress = reg;
    masterXfer.subaddressSize = 1;
    masterXfer.data = &writedata;
    masterXfer.dataSize = 1;
    if(i2c_master_transfer(I2C2, &masterXfer))
        status = 1;
    return status;
}

/* 从FT5426读取一个字节数据 */
unsigned char ft5426_read_byte(unsigned char addr, unsigned char reg)
{
    unsigned char val  = 0;
    
    struct i2c_transfer masterXfer;

    masterXfer.slaveAddress = addr;
    masterXfer.direction = kI2C_Read;
    masterXfer.subaddress = reg;
    masterXfer.subaddressSize = 1;
    masterXfer.data = &val;
    masterXfer.dataSize = 1;
    i2c_master_transfer(I2C2, &masterXfer);
    return val;
}

/* 从FT5426读取多个寄存器数据 */
void ft5426_read_len(unsigned char addr, unsigned char reg, 
                             unsigned char len,  unsigned char *buf)
{
    struct i2c_transfer masterXfer;

    masterXfer.slaveAddress = addr;
    masterXfer.direction = kI2C_Read;
    masterXfer.subaddress = reg;
    masterXfer.subaddressSize = 1;
    masterXfer.data = buf;
    masterXfer.dataSize = len;
    i2c_master_transfer(I2C2, &masterXfer);
}

/* 读取触摸坐标信息 */
void ft5426_read_tpcoord(void)
{
    unsigned char i = 0;
    unsigned char type = 0;
    unsigned char pointbuf[FT5426_XYCOORDREG_NUM];

    ft5426_dev.point_num = ft5426_read_byte(FT5426_ADDR, FT5426_TD_STATUS);

    /* 读取触摸点信息寄存器 */
    ft5426_read_len(FT5426_ADDR, FT5426_TOUCH1_XH, 
                    FT5426_XYCOORDREG_NUM, pointbuf);

    for(i = 0; i < ft5426_dev.point_num; i++) {
        unsigned char *buf = &pointbuf[i * 6];

        ft5426_dev.y[i] = ((buf[0] << 8) | buf[1]) & 0xfff;
        ft5426_dev.x[i] = ((buf[2] << 8) | buf[3]) & 0xfff;

        type = buf[0] >> 6;
        if(type == FT5426_TOUCH_EVENT_DOWN) {

        } else if(type == FT5426_TOUCH_EVENT_UP) {
            
        } else if(type == FT5426_TOUCH_EVENT_ON) {

        } else {

        }
    }
}


