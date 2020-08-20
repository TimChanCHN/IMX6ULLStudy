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

#define GPIOBEEP_CNT      1                /* 設備個數 */
#define GPIOBEEP_NAME     "gpiobeep"         /* 名字 */

#define BEEP_OFF         1
#define BEEP_ON          0

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
struct gpiobeep_dev{
    dev_t devid;                /* 设备号 */
    struct cdev cdev;           /* cdev */
    struct class *class;        /* 类 */
    struct device *device;      /* 设备 */
    int major;
    int minor;
    struct device_node *nd;
    int led_gpio;
    int beep_gpio;
};

struct gpiobeep_dev gpiobeep;

static int beep_open(struct inode *inode, struct file *filp)
{
    printk("beep.ko open!\r\n");
    filp->private_data = &gpiobeep;        /* 设置私有数据 */
    return 0;
}

static int beep_release(struct inode *inode, struct file *filp)
{
    printk("beep.ko close!\r\n");
    return 0;
}

static ssize_t beep_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offset)
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

static ssize_t beep_write(struct file *filp, const char __user *buff, size_t cnt, loff_t *offset)
{
    int ret = 0;
    unsigned char databuf[1];
    unsigned char beepstate;

    struct gpiobeep_dev *dev = filp->private_data;

    ret = copy_from_user(databuf, buff, cnt);
    if (ret < 0)
    {
        printk("kernel write fail!\r\n");
        return -EFAULT;
    }

    beepstate = databuf[0];
    if (beepstate == BEEP_ON)
    {
        gpio_set_value(dev->beep_gpio, 0);
    }else{
        gpio_set_value(dev->beep_gpio, 1);
    }
    
    return 0;
}

static struct file_operations gpiobeep_fops = {
    .owner      = THIS_MODULE,
    .open       =   beep_open,
    .release    =   beep_release,
    .read       =   beep_read,
    .write      =   beep_write,
};

static int __init beep_init(void)
{
    u32 val = 0;
    int ret;
    u32 regdata[14];
    const char* str;
    struct property *proper;

    /* 获取设备数中的属性数据 */
    /* 1. 获取设备节点:gpiobeep */
    gpiobeep.nd = of_find_node_by_path("/gpiobeep");
    if (gpiobeep.nd == NULL){
        printk("gpiobeep node can not found.\r\n");
        return -EINVAL;
    }else{
        printk("gpiobeep node has been found.\r\n");
    }

    /* 2. 获取设备树中该节点的gpio属性，得到BEEP使用的BEEP编号 */
    gpiobeep.beep_gpio = of_get_named_gpio(gpiobeep.nd, "beep-gpios", 0);
    if (gpiobeep.beep_gpio < 0)
    {
        printk("can not get beep-gpio");
        return -EINVAL;
    }
    printk("beep-gpio num = %d\r\n", gpiobeep.beep_gpio);

    /* 3.设置GPIO1——IO03为输出且输出高电平，默认关闭BEEP */
    ret = gpio_direction_output(gpiobeep.beep_gpio, 1);
    if (ret < 0)
    {
        printk("can't set beep-gpio");
    }

    /* 注冊字符設備驅動 */
    if (gpiobeep.major){       /* 已有设备号 */
        gpiobeep.devid = MKDEV(gpiobeep.major, 0);
        register_chrdev_region(gpiobeep.devid, GPIOBEEP_CNT, GPIOBEEP_NAME);
    }else{
        alloc_chrdev_region(&gpiobeep.devid, 0, GPIOBEEP_CNT, GPIOBEEP_NAME);  /* 申请设备号 */
        gpiobeep.major = MAJOR(gpiobeep.devid);       /* 获得主设备号 */
        gpiobeep.minor = MINOR(gpiobeep.devid);       /* 获得次设备号 */
    }
    printk("gpiobeep major = %d, minor = %d\r\n", gpiobeep.major, gpiobeep.minor);

    /* 初始化cdev */
    gpiobeep.cdev.owner = THIS_MODULE;
    cdev_init(&gpiobeep.cdev, &gpiobeep_fops);

    /* 添加cdev */
    cdev_add(&gpiobeep.cdev, gpiobeep.devid, GPIOBEEP_CNT);

    /* 创建类 */
    gpiobeep.class = class_create(THIS_MODULE, GPIOBEEP_NAME);
    if (IS_ERR(gpiobeep.class))
    {
        return PTR_ERR(gpiobeep.class);
    }

    /* 创建设备 */
    gpiobeep.device = device_create(gpiobeep.class, NULL, gpiobeep.devid, NULL, GPIOBEEP_NAME);

    if (IS_ERR(gpiobeep.device))
    {
        return PTR_ERR(gpiobeep.device);
    }

    printk("beep init.\n\r");
    return 0;
}

static void __exit beep_exit(void)
{
    /* 注销字符设备 */
    cdev_del(&gpiobeep.cdev);
    unregister_chrdev_region(gpiobeep.devid, GPIOBEEP_CNT);

    device_destroy(gpiobeep.class, gpiobeep.devid);
    class_destroy(gpiobeep.class);

    printk("beep exit.\n\r");
}

module_init(beep_init);
module_exit(beep_exit);

MODULE_LICENSE("GPL");
