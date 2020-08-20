#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/* 
    实验目的:一次只允许一个应用访问LED
 */

#define GPIOLED_CNT      1                /* 設備個數 */
#define GPIOLED_NAME     "gpioled"         /* 名字 */

#define LED_OFF         0
#define LED_ON          1

/* 寄存器物理地址 */
#define CCM_CCGR1_BASE          (0X020C406C)
#define SW_MUX_GPIO1_IO03_BASE  (0X020E0068)
#define SW_PAD_GPIO1_IO03_BASE  (0X020E02F4)
#define GPIO1_DR_BASE           (0X0209C000)
#define GPIO1_GDIR_BASE         (0X0209C004)

/* 映射後的寄存器虛擬地址指針 */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

static char readbuff[100];
static char writebuff[100];
static char kerneldata[] = "kernel data";

/* newchrdev设备结构体 */
struct gpioled_dev{
    dev_t devid;                /* 设备号 */
    struct cdev cdev;           /* cdev */
    struct class *class;        /* 类 */
    struct device *device;      /* 设备 */
    int major;
    int minor;
    struct device_node *nd;
    int led_gpio;
    int dev_stas;               /* 设备状态，0：未使用，>0:设备已经被使用 */
    spinlock_t lock;            /* 自选锁 */
};

struct gpioled_dev gpioled;

static void led_switch(u8 sta)
{
    u32 val = 0;
    if (sta == LED_ON)
    {
        val = readl(GPIO1_DR);
        val &= ~(1 << 3);
        writel(val, GPIO1_DR);
    }
    else if (sta == LED_OFF)
    {
        val = readl(GPIO1_DR);
        val |= (1 << 3);
        writel(val, GPIO1_DR);
    }

}

static int led_open(struct inode *inode, struct file *filp)
{
    unsigned long flags;
    filp->private_data = &gpioled;        /* 设置私有数据 */
    
    spin_lock_irqsave(&gpioled.lock, flags);
    if (gpioled.dev_stas){                  /* 设备被使用 */
        spin_unlock_irqrestore(&gpioled.lock, flags);
        return -EBUSY;
    }
    gpioled.dev_stas++;
    spin_unlock_irqrestore(&gpioled.lock, flags);
    
    return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
    unsigned long flags;
    struct gpioled_dev *dev = filp->private_data;

    /* 关闭驱动文件时，使原子量为1,释放原子变量 */
    spin_lock_irqsave(&dev->lock, flags);
    if (dev->dev_stas){
        dev->dev_stas--;
    }
    spin_unlock_irqrestore(&dev->lock, flags);
    printk("led.ko close!\r\n");
    return 0;
}

static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offset)
{
    int ret = 0;
    memcpy(readbuff, kerneldata, sizeof(kerneldata));
    ret = copy_to_user(buf, readbuff, cnt);
    if (ret < 0)
    {
        printk("read kerneldata fail\r\n");
    }

    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buff, size_t cnt, loff_t *offset)
{
    int ret = 0;
    unsigned char databuf[1];
    unsigned char ledstate;

    struct gpioled_dev *dev = filp->private_data;

    ret = copy_from_user(databuf, buff, cnt);
    if (ret < 0)
    {
        printk("kernel write fail!\r\n");
        return -EFAULT;
    }

    ledstate = databuf[0];
    if (ledstate == LED_ON)
    {
        gpio_set_value(dev->led_gpio, 0);
    }else{
        gpio_set_value(dev->led_gpio, 1);
    }
    
    return 0;
}

static struct file_operations gpioled_fops = {
    .owner      = THIS_MODULE,
    .open       =   led_open,
    .release    =   led_release,
    .read       =   led_read,
    .write      =   led_write,
};

static int __init led_init(void)
{
    u32 val = 0;
    int ret;
    u32 regdata[14];
    const char* str;
    struct property *proper;

    /* 初始化原子变量 */
    spin_lock_init(&gpioled.lock);

    /* 获取设备数中的属性数据 */
    /* 1. 获取设备节点:gpioled */
    gpioled.nd = of_find_node_by_path("/gpioled");
    if (gpioled.nd == NULL){
        printk("gpioled node can not found.\r\n");
        return -EINVAL;
    }else{
        printk("gpioled node has been found.\r\n");
    }

    /* 2. 获取设备树中该节点的gpio属性，得到LED使用的LED编号 */
    gpioled.led_gpio = of_get_named_gpio(gpioled.nd, "led-gpio", 0);
    if (gpioled.led_gpio < 0)
    {
        printk("can not get led-gpio");
        return -EINVAL;
    }
    printk("led-gpio num = %d\r\n", gpioled.led_gpio);

    /* 3.设置GPIO1——IO03为输出且输出高电平，默认关闭LED灯 */
    ret = gpio_direction_output(gpioled.led_gpio, 1);
    if (ret < 0)
    {
        printk("can't set led-gpio");
    }

    /* 注冊字符設備驅動 */
    if (gpioled.major){       /* 已有设备号 */
        gpioled.devid = MKDEV(gpioled.major, 0);
        register_chrdev_region(gpioled.devid, GPIOLED_CNT, GPIOLED_NAME);
    }else{
        alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME);  /* 申请设备号 */
        gpioled.major = MAJOR(gpioled.devid);       /* 获得主设备号 */
        gpioled.minor = MINOR(gpioled.devid);       /* 获得次设备号 */
    }
    printk("gpioled major = %d, minor = %d\r\n", gpioled.major, gpioled.minor);

    /* 初始化cdev */
    gpioled.cdev.owner = THIS_MODULE;
    cdev_init(&gpioled.cdev, &gpioled_fops);

    /* 添加cdev */
    cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT);

    /* 创建类 */
    gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
    if (IS_ERR(gpioled.class))
    {
        return PTR_ERR(gpioled.class);
    }

    /* 创建设备 */
    gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, GPIOLED_NAME);

    if (IS_ERR(gpioled.device))
    {
        return PTR_ERR(gpioled.device);
    }

    printk("led init.\n\r");
    return 0;
}

static void __exit led_exit(void)
{
    /* 注销字符设备 */
    cdev_del(&gpioled.cdev);
    unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);

    device_destroy(gpioled.class, gpioled.devid);
    class_destroy(gpioled.class);

    printk("led exit.\n\r");
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
