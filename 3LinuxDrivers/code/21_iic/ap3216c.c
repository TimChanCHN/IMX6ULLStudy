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
#include <linux/platform_device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "ap3216.h"

#define AP3216C_CNT  1
#define AP3216C_NAME "ap3216c"

/* ap3216c设备结构体 */
struct ap3216c_dev{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    void* private_data;
    unsigned short ir, als, ps;
};

struct ap3216c_dev ap3216c;

/* 从AP3216C读取多个寄存器数据 */
static int ap3216c_read_regs(struct ap3216c_dev *dev, u8 reg, void *val, int len)
{
    int ret;
    struct i2c_msg msg[2];
    struct i2c_client *client = (struct i2c_client*)dev->private_data;

    /* 发送要读取的首地址 */
    msg[0].addr     = client->addr;
    msg[0].flags    = 0;                /* 标记为发送数据/写操作 */
    msg[0].buf      = &reg;             /* 读取的首地址 */
    msg[0].len      = 1;                /* reg长度 */

    msg[1].addr     = client->addr;
    msg[1].flags    = I2C_M_RD;         /* 标记成读数据 */
    msg[1].buf      = val;              /* 读取数据的缓冲区 */
    msg[1].len      = len;              /* 读取的数据长度 */

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret == 2){
        ret = 0;
    }else{
        printk("i2c rd faiap3216c = %d, reg = %06x, len=%d\n",ret, reg, len);
        ret = -EREMOTEIO;
    }

    return ret;
}

/* 从AP3216C写多个数据 */
static int ap3216c_write_regs(struct ap3216c_dev *dev, u8 reg, void *buf, int len)
{
    u8 b[256];                      // 要写入的寄存器地址以及数据内容
    struct i2c_msg msg;
    struct i2c_client *client = (struct i2c_client*)dev->private_data;

    b[0] = reg;                     /* 寄存器首地址 */
    memcpy(&b[1], buf, len);        /* 将要写入的数据存到b数组里 */

    msg.addr    = client->addr;
    msg.flags   = 0;
    msg.buf     = b;
    msg.len     = len+1;

    return i2c_transfer(client->adapter, &msg, 1);
}

/* 从AP3216C读一个寄存器 */
static unsigned char ap3216c_read_reg(struct ap3216c_dev *dev, u8 reg)
{
    u8 data = 0;

    ap3216c_read_regs(dev, reg, &data, 1);
    return data;
#if 0
    struct i2c_client *client = (struct i2c_client*)dev->private_data;
    
    return i2c_smbus_read_byte_data(client, reg);
#endif
}

/* 从AP3216C写一个寄存器 */
static void ap3216c_write_reg(struct ap3216c_dev *dev, u8 reg, u8 data)
{
    u8 buf = 0;
    buf = data;
    ap3216c_write_regs(dev, reg, &buf, 1);
}

/* 读取AP3216C数据，同时打开 ALS,IR+PS 的话两次数据读取的间隔要大于 112.5ms */
static void ap3216c_readdata(struct ap3216c_dev *dev)
{
    unsigned char i = 0;
    unsigned char buf[6];

    for (i = 0; i < 6; i++){
        buf[i] = ap3216c_read_reg(dev, AP3216C_IRDATALOW+i);
    }

    /* ir数据 */
    if (buf[0] & 0x80){
        dev->ir = 0;
    } else {
        dev->ir = ((unsigned short)buf[1] << 2) | (buf[0] & 0x03);
    }
    /* ALS数据 */
    dev->als = ((unsigned short)buf[3] << 8) | buf[2];
    /* ps数据 */
    if (buf[4] & 0x40){
        dev->ps = 0;
    } else {
        dev->ps = ((unsigned short)(buf[5] & 0x3f)<<4) | (buf[4] & 0x0f);
    }

}

/* 打开设备 */
static int ap3216c_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &ap3216c;

    /* 初始化AP3216C */
    ap3216c_write_reg(&ap3216c, AP3216C_SYSTEMCONG, 0X04);
    mdelay(50);
    ap3216c_write_reg(&ap3216c, AP3216C_SYSTEMCONG, 0x03);
    return 0;
}

static ssize_t ap3216c_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
    short data[3];
    long err = 0;

    struct ap3216c_dev *dev = (struct ap3216c_dev*)filp->private_data;

    ap3216c_readdata(dev);
    data[0] = dev->ir;
    data[1] = dev->als;
    data[2] = dev->ps;
    err = copy_to_user(buf, data, sizeof(data));

    return 0;
}

/* fops */
static struct file_operations ap3216c_fops={
    .owner  = THIS_MODULE,
    .open   = ap3216c_open,
    .read   = ap3216c_read,
};

/* platform probe 函数 */
static int ap3216c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    /* 注册字符设备驱动 */
    if (ap3216c.major){
        ap3216c.devid = MKDEV(ap3216c.major, 0);
        register_chrdev_region(ap3216c.devid, AP3216C_CNT, AP3216C_NAME);
    } else{
        alloc_chrdev_region(&ap3216c.devid, 0, AP3216C_CNT, AP3216C_NAME);
        ap3216c.major = MAJOR(ap3216c.devid);
    }

    /* 初始化cdev */
    ap3216c.cdev.owner = THIS_MODULE;
    cdev_init(&ap3216c.cdev, &ap3216c_fops);
    cdev_add(&ap3216c.cdev, ap3216c.devid, AP3216C_CNT);

    /* 创建类 */
    ap3216c.class = class_create(THIS_MODULE, AP3216C_NAME);
    if (IS_ERR(ap3216c.class)){
        return PTR_ERR(ap3216c.class);
    }
    ap3216c.device = device_create(ap3216c.class, NULL, ap3216c.devid, NULL, AP3216C_NAME);
    if (IS_ERR(ap3216c.device)){
        return PTR_ERR(ap3216c.device);
    }

    ap3216c.private_data = client;
    printk("i2c device match!!\r\n");

    return 0;
}

/* remove函数 */
static int ap3216c_remove(struct i2c_client *client)
{
    cdev_del(&ap3216c.cdev);
    unregister_chrdev_region(ap3216c.devid, AP3216C_CNT);
    device_destroy(ap3216c.class, ap3216c.devid);
    class_destroy(ap3216c.class);
    return 0;
}

static const struct i2c_device_id ap3216c_id[]={
    {"alientek,ap3216c", 0},
    {}
};

static const struct of_device_id ap3216c_of_match[] = {
    {.compatible = "alientek,ap3216c"},
    {}
};

/* i2c驱动结构体 */
static struct i2c_driver ap3216c_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .of_match_table = ap3216c_of_match,
        .name = "imx6ul-ap3216c",
    },
    .probe = ap3216c_probe,
    .remove = ap3216c_remove,
    .id_table = ap3216c_id,
};

static int __init ap3216cdriver_init(void)
{
    int ret;

    ret = i2c_add_driver(&ap3216c_driver);
    return ret;
}

static void __exit ap3216cdriver_exit(void)
{
    i2c_del_driver(&ap3216c_driver);
}

module_init(ap3216cdriver_init);
module_exit(ap3216cdriver_exit);
MODULE_LICENSE("GPL");

