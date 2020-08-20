#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/platform_device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "icm20608reg.h"

#define ICM20608_CNT  1
#define ICM20608_NAME "icm20608"

/* icm20608设备结构体 */
struct icm20608_dev{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    int major;
    void* private_data;
    int cs_gpio;
    signed int gyro_x_adc;          /* 陀螺仪的值 */
    signed int gyro_y_adc;          /* 陀螺仪的值 */
    signed int gyro_z_adc;          /* 陀螺仪的值 */
    signed int accel_x_adc;          /* 加速度计值 */
    signed int accel_y_adc;          /* 加速度计值 */
    signed int accel_z_adc;          /* 加速度计值 */
    signed int temp_adc;            /* 温度计的值 */
};

struct icm20608_dev icm20608;

/* 从ICM20608读取多个寄存器数据 */
static int icm20608_read_regs(struct icm20608_dev *dev, u8 reg, void *buf, int len)
{
    int ret;
    unsigned char txdata[len];
    struct spi_message m;
    struct spi_transfer *t;
    struct spi_device *spi = (struct spi_device*)dev->private_data;

    gpio_set_value(dev->cs_gpio, 0);
    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);       // 是自带清零功能的kmalloc

    /* 发送要读的寄存器地址 */
    txdata[0] = reg | 0x80;             // 读数据时，bit7置1
    t->tx_buf = txdata;                 // 要发送的数据
    t->len = 1;                         // 1个字节的数据长度
    spi_message_init(&m);               // 初始化spi_message
    spi_message_add_tail(t, &m);        // 将spi_transfer添加到spi_message
    ret = spi_sync(spi, &m);               

    /* 读数据 */
    txdata[0] = 0xff;
    t->rx_buf = buf;                         
    t->len = len;
    spi_message_init(&m);
    spi_message_add_tail(t, &m);
    ret = spi_sync(spi, &m);

    kfree(t);
    gpio_set_value(dev->cs_gpio, 1);

    return ret;
}

/* 从ICM20608写多个数据 */
static s32 icm20608_write_regs(struct icm20608_dev *dev, u8 reg, u8 *buf, u8 len)
{
    int ret;
    unsigned char txdata[len];
    struct spi_message m;
    struct spi_transfer *t;
    struct spi_device *spi = (struct spi_device*)dev->private_data;

    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
    gpio_set_value(dev->cs_gpio, 0);

    /* 发送写地址 */
    txdata[0] = reg & ~0x80;            // 写数据时，bit7清零
    t->tx_buf = txdata;
    t->len = 1;
    spi_message_init(&m);
    spi_message_add_tail(t, &m);
    ret = spi_sync(spi, &m);

    /* 发送写入的数据 */
    t->tx_buf = buf;
    t->len = len;
    spi_message_init(&m);
    spi_message_add_tail(t, &m);
    ret = spi_sync(spi, &m);

    kfree(t);
    gpio_set_value(dev->cs_gpio, 1);

    return ret;
}

/* 从ICM20608读一个寄存器 */
static unsigned char icm20608_read_onereg(struct icm20608_dev *dev, u8 reg)
{
    u8 data = 0;
    icm20608_read_regs(dev, reg, &data, 1);
    return data;
}

/* 从ICM20608写一个寄存器 */
static void icm20608_write_onereg(struct icm20608_dev *dev, u8 reg, u8 data)
{
    u8 buf = 0;
    buf = data;
    icm20608_write_regs(dev, reg, &buf, 1);
}

/* 读取ICM20608数据，同时打开 ALS,IR+PS 的话两次数据读取的间隔要大于 112.5ms */
static void icm20608_readdata(struct icm20608_dev *dev)
{
    unsigned char data[14];
    icm20608_read_regs(dev, ICM20_ACCEL_XOUT_H, data, 14);

    dev->accel_x_adc = (signed short)((data[0] << 8) | data[1]);
    dev->accel_y_adc = (signed short)((data[2] << 8) | data[3]);
    dev->accel_z_adc = (signed short)((data[4] << 8) | data[5]);
    dev->temp_adc = (signed short)((data[6] << 8) | data[7]);
    dev->gyro_x_adc = (signed short)((data[8] << 8) | data[9]);
    dev->gyro_y_adc = (signed short)((data[10] << 8) | data[11]);
    dev->gyro_z_adc = (signed short)((data[12] << 8) | data[13]);

}

static void icm20608_reginit(void)
{
    u8 value = 0;
    icm20608_write_onereg(&icm20608, ICM20_PWR_MGMT_1, 0x80);
    mdelay(50);
    icm20608_write_onereg(&icm20608, ICM20_PWR_MGMT_1, 0x01);
    mdelay(50);

    value = icm20608_read_onereg(&icm20608, ICM20_WHO_AM_I);
    printk("ICM20608 id = %#X\r\n", value);

    icm20608_write_onereg(&icm20608, ICM20_SMPLRT_DIV, 0x00);
    icm20608_write_onereg(&icm20608, ICM20_GYRO_CONFIG, 0x18);
    icm20608_write_onereg(&icm20608, ICM20_ACCEL_CONFIG, 0x18);
    icm20608_write_onereg(&icm20608, ICM20_CONFIG, 0x04);
    icm20608_write_onereg(&icm20608, ICM20_ACCEL_CONFIG2, 0x04);
    icm20608_write_onereg(&icm20608, ICM20_PWR_MGMT_2, 0x00);
    icm20608_write_onereg(&icm20608, ICM20_LP_MODE_CFG, 0x00);
    icm20608_write_onereg(&icm20608, ICM20_FIFO_EN, 0x00);
}

/* 打开设备 */
static int icm20608_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &icm20608;

    return 0;
}

static ssize_t icm20608_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
    signed int data[7];
    long err = 0;
    struct icm20608_dev *dev = (struct icm20608_dev*)filp->private_data;

    icm20608_readdata(dev);
    data[0] = dev->gyro_x_adc;
    data[1] = dev->gyro_y_adc;
    data[2] = dev->gyro_z_adc;
    data[3] = dev->accel_x_adc;
    data[4] = dev->accel_y_adc;
    data[5] = dev->accel_z_adc;
    data[6] = dev->temp_adc;
    err = copy_to_user(buf, data, sizeof(data));

    return 0;
}

/* fops */
static struct file_operations icm20608_fops={
    .owner  = THIS_MODULE,
    .open   = icm20608_open,
    .read   = icm20608_read,
};

/* platform probe 函数 */
static int icm20608_probe(struct spi_device *spi)
{
    int ret = 0;

    /* 注册字符设备驱动 */
    if (icm20608.major){
        icm20608.devid = MKDEV(icm20608.major, 0);
        register_chrdev_region(icm20608.devid, ICM20608_CNT, ICM20608_NAME);
    } else{
        alloc_chrdev_region(&icm20608.devid, 0, ICM20608_CNT, ICM20608_NAME);
        icm20608.major = MAJOR(icm20608.devid);
    }

    /* 初始化cdev */
    icm20608.cdev.owner = THIS_MODULE;
    cdev_init(&icm20608.cdev, &icm20608_fops);
    cdev_add(&icm20608.cdev, icm20608.devid, ICM20608_CNT);

    /* 创建类 */
    icm20608.class = class_create(THIS_MODULE, ICM20608_NAME);
    if (IS_ERR(icm20608.class)){
        return PTR_ERR(icm20608.class);
    }
    icm20608.device = device_create(icm20608.class, NULL, icm20608.devid, NULL, ICM20608_NAME);
    if (IS_ERR(icm20608.device)){
        return PTR_ERR(icm20608.device);
    }
    printk("xxxx\r\n");
    /* 从设备树中获取cs片选信号 */
    icm20608.nd = of_find_node_by_path("/soc/aips-bus@02000000/spba-bus@02000000/ecspi@02010000");
    
    if (icm20608.nd == NULL){
        printk("ecspi3 node not find.\r\n");
        return -EINVAL;
    }

    /* 从node中获取GPIO属性 */
    icm20608.cs_gpio = of_get_named_gpio(icm20608.nd, "cs-gpio", 0);
    if (icm20608.cs_gpio < 0){
        printk("can't get cs-gpio.\r\n");
        return -EINVAL;
    }

    /* 设置片选为输出 */
    ret = gpio_direction_output(icm20608.cs_gpio, 1);
    if (ret < 0){
        printk("can't set gpio.\r\n");
    }

    /* 初始化spi_device */
    spi->mode = SPI_MODE_0;
    spi_setup(spi);
    icm20608.private_data = spi;

    /* 初始化ICM20608 */
    icm20608_reginit();

    printk("i2c device match!!\r\n");

    return 0;
}

/* remove函数 */
static int icm20608_remove(struct spi_device *spi)
{
    cdev_del(&icm20608.cdev);
    unregister_chrdev_region(icm20608.devid, ICM20608_CNT);
    device_destroy(icm20608.class, icm20608.devid);
    class_destroy(icm20608.class);
    return 0;
}

/* 设备信息 */
static const struct spi_device_id icm20608_id[]={
    {"alientek,icm20608", 0},
    {}
};

static const struct of_device_id icm20608_of_match[] = {
    {.compatible = "alientek,icm20608"},
    {}
};

/* spi驱动结构体 */
static struct spi_driver icm20608_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .of_match_table = icm20608_of_match,
        .name = "imx6ul-icm20608",
    },
    .probe = icm20608_probe,
    .remove = icm20608_remove,
    .id_table = icm20608_id,
};

static int __init icm20608driver_init(void)
{
    return spi_register_driver(&icm20608_driver);
}

static void __exit icm20608driver_exit(void)
{
    spi_unregister_driver(&icm20608_driver);
}

module_init(icm20608driver_init);
module_exit(icm20608driver_exit);
MODULE_LICENSE("GPL");

