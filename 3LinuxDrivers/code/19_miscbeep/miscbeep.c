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
#include <linux/miscdevice.h>
#include <asm/mach/map.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define MISCBEEP_NAME   "miscbeep"
#define MISCBEEP_MINOR  144             
#define BEEPOFF         0
#define BEEPON          1

/* miscbeep设备结构体 */
struct miscbeep_dev{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *node;
    int beep_gpio;
};

struct miscbeep_dev miscbeep;

/* 打开设备 */
static int miscbeep_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &miscbeep;
    return 0;
}

static ssize_t miscbeep_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offset)
{
    int retval;
    unsigned char databuf[1];
    unsigned char beepsta;
    struct miscbeep_dev *dev = (struct miscbeep_dev*)filp->private_data;

    retval = copy_from_user(databuf, buf, cnt);
    if (retval < 0){
        return -EFAULT;
    }

    beepsta = databuf[0];
    if (beepsta == BEEPON){
        gpio_set_value(dev->beep_gpio, 0);
    }else{
        gpio_set_value(dev->beep_gpio, 1);
    }
}

/* fops */
static struct file_operations beep_fops={
    .owner  = THIS_MODULE,
    .open   = miscbeep_open,
    .write  = miscbeep_write,
};

/* misc设备结构体 */
static struct miscdevice beep_miscdev = {
    .minor = MISCBEEP_MINOR,
    .name   = MISCBEEP_NAME,
    .fops   = &beep_fops,
};

/* platform probe 函数 */
static int miscbeep_probe(struct platform_device *dev)
{
    int ret = 0;

    printk("beep driver and device was matched!\r\n");

    /* 初始化IO */
    miscbeep.node = of_find_node_by_path("/gpiobeep");
    if (miscbeep.node == NULL)
    {
        printk("beep node not find\r\n");
        return -EINVAL;
    }

    miscbeep.beep_gpio = of_get_named_gpio(miscbeep.node, "beep-gpios", 0);
    if (miscbeep.beep_gpio < 0){
        printk("can not get beep_gpio.\r\n");
        return -EINVAL;
    }

    // gpio_request(miscbeep.beep_gpio, "beep");
    gpio_direction_output(miscbeep.beep_gpio, 1);

    /* 注册misc */
    ret = misc_register(&beep_miscdev);
    if (ret < 0)
    {
        printk("misc device register failed!\r\n");
        return -EFAULT;
    }
    return 0;
}

/* remove函数 */
static int miscbeep_remove(struct platform_device *dev)
{
    gpio_set_value(miscbeep.beep_gpio, 1);    

    misc_deregister(&beep_miscdev);
    return 0;
}

/* 匹配列表 */
static const struct of_device_id led_of_match[] = {
    {.compatible = "atk_alpha-beep"},
    {}
};

/* platform结构体 */
static struct platform_driver led_driver = {
    .driver = {
        .name = "imx6ul-beep",
        .of_match_table = led_of_match,
    },
    .probe = miscbeep_probe,
    .remove = miscbeep_remove,
};

static int __init miscbeepdriver_init(void)
{
    return platform_driver_register(&led_driver);
}

static void __exit miscbeepdriver_exit(void)
{
    return platform_driver_unregister(&led_driver);
}

module_init(miscbeepdriver_init);
module_exit(miscbeepdriver_exit);
MODULE_LICENSE("GPL");

