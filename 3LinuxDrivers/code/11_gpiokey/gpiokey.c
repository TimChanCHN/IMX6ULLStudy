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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/* 
    实验目的:一次只允许一个应用访问LED
 */

#define GPIOKEY_CNT      1                /* 設備個數 */
#define GPIOKEY_NAME     "gpioKEY"         /* 名字 */

#define KEY0VALUE    0XF0
#define INVAKEY      0X00

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
struct gpiokey_dev{
    dev_t devid;                /* 设备号 */
    struct cdev cdev;           /* cdev */
    struct class *class;        /* 类 */
    struct device *device;      /* 设备 */
    int major;
    int minor;
    struct device_node *nd;
    int key_gpio;
    atomic_t keyvalue;              /* 原子变量 */
};

struct gpiokey_dev gpiokey;

static int keyio_init(void)
{
    /* 获取设备数中的属性数据 */
    /* 1. 获取设备节点:gpiokey */
    gpiokey.nd = of_find_node_by_path("/gpiokey");
    if (gpiokey.nd == NULL){
        printk("gpiokey node can not found.\r\n");
        return -EINVAL;
    }else{
        printk("gpiokey node has been found.\r\n");
    }

    /* 2. 获取设备树中该节点的gpio属性，得到LED使用的LED编号 */
    gpiokey.key_gpio = of_get_named_gpio(gpiokey.nd, "key-gpios", 0);
    if (gpiokey.key_gpio < 0)
    {
        printk("can not get key-gpios");
        return -EINVAL;
    }
    printk("key-gpios num = %d\r\n", gpiokey.key_gpio);

    /* 3. 初始化key所使用的gpio */
    gpio_request(gpiokey.key_gpio, "key0");
    gpio_direction_input(gpiokey.key_gpio);
    return 0;
}

static int key_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &gpiokey;        /* 设置私有数据 */

    keyio_init();
    return 0;
}

static int key_release(struct inode *inode, struct file *filp)
{
    printk("led.ko close!\r\n");
    return 0;
}

static ssize_t key_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offset)
{
    int ret = 0;
    unsigned char value;
    struct gpiokey_dev *dev = filp->private_data;

    if (gpio_get_value(dev->key_gpio) == 0){
        while (!gpio_get_value(dev->key_gpio));     /* 等待按键释放 */
        atomic_set(&dev->keyvalue, KEY0VALUE);
    }else{
        atomic_set(&dev->keyvalue, INVAKEY);
    }

    value = atomic_read(&dev->keyvalue);
    ret = copy_to_user(buf, &value, sizeof(value));
    if (ret < 0)
    {
        printk("read keyvalue fail\r\n");
    }

    return 0;
}

static struct file_operations gpiokey_fops = {
    .owner      = THIS_MODULE,
    .open       =   key_open,
    .release    =   key_release,
    .read       =   key_read,
};

static int __init gpiokey_init(void)
{
    u32 val = 0;
    int ret;
    u32 regdata[14];
    const char* str;
    struct property *proper;

    /* 初始化原子变量 */
    atomic_set(&gpiokey.keyvalue, INVAKEY);


    /* 注冊字符設備驅動 */
    if (gpiokey.major){       /* 已有设备号 */
        gpiokey.devid = MKDEV(gpiokey.major, 0);
        register_chrdev_region(gpiokey.devid, GPIOKEY_CNT, GPIOKEY_NAME);
    }else{
        alloc_chrdev_region(&gpiokey.devid, 0, GPIOKEY_CNT, GPIOKEY_NAME);  /* 申请设备号 */
        gpiokey.major = MAJOR(gpiokey.devid);       /* 获得主设备号 */
        gpiokey.minor = MINOR(gpiokey.devid);       /* 获得次设备号 */
    }
    printk("gpiokey major = %d, minor = %d\r\n", gpiokey.major, gpiokey.minor);

    /* 初始化cdev */
    gpiokey.cdev.owner = THIS_MODULE;
    cdev_init(&gpiokey.cdev, &gpiokey_fops);

    /* 添加cdev */
    cdev_add(&gpiokey.cdev, gpiokey.devid, GPIOKEY_CNT);

    /* 创建类 */
    gpiokey.class = class_create(THIS_MODULE, GPIOKEY_NAME);
    if (IS_ERR(gpiokey.class))
    {
        return PTR_ERR(gpiokey.class);
    }

    /* 创建设备 */
    gpiokey.device = device_create(gpiokey.class, NULL, gpiokey.devid, NULL, GPIOKEY_NAME);

    if (IS_ERR(gpiokey.device))
    {
        return PTR_ERR(gpiokey.device);
    }

    printk("key init.\n\r");
    return 0;
}

static void __exit gpiokey_exit(void)
{
    /* 注销字符设备 */
    cdev_del(&gpiokey.cdev);
    unregister_chrdev_region(gpiokey.devid, GPIOKEY_CNT);

    device_destroy(gpiokey.class, gpiokey.devid);
    class_destroy(gpiokey.class);

    printk("key exit.\n\r");
}

module_init(gpiokey_init);
module_exit(gpiokey_exit);

MODULE_LICENSE("GPL");
