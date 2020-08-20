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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/* 
    实验目的:当按键按下时，启动定时器，过了一段时间，若按键依然按下，则认为按键按下
 */

#define IMX6UIRQ_CNT      1                /* 設備個數 */
#define IMX6UIRQ_NAME     "noblockio"         /* 名字 */

#define KEY0_VALUE         0X01
#define INVAKEY            0XFF
#define KEY_NUM            1

/* 中断irq描述结构体 */
struct irq_keydesc{
    int gpio;
    int irqnum;
    unsigned char value;        // 键值
    char name[10];
    irqreturn_t (*handler)(int, void*);
};

/* newchrdev设备结构体 */
struct imx6uirq_dev{
    dev_t devid;                /* 设备号 */
    struct cdev cdev;           /* cdev */
    struct class *class;        /* 类 */
    struct device *device;      /* 设备 */
    int major;
    int minor;
    struct device_node *nd;
    atomic_t keyvalue;          /* 有效的按键值 */
    atomic_t releasekey;        /* 标记是否完成一次完成的按钮 */
    struct timer_list timer;        /* 定时器 */
    struct irq_keydesc irq_keydesc[KEY_NUM];        /* 按键描述数组 */
    unsigned char curkeynum;                        /* 当前按键号 */
    wait_queue_head_t r_wait;            /* 等待队列头 */
};

struct imx6uirq_dev imx6uirq;

/* 按键0中断服务函数 */
static irqreturn_t key0_handler(int irq, void* dev_id)
{
    struct imx6uirq_dev* dev = (struct imx6uirq_dev*)dev_id;

    dev->curkeynum = 0;
    dev->timer.data = (volatile long)dev_id;
    mod_timer(&dev->timer, jiffies+msecs_to_jiffies(10));
    return IRQ_RETVAL(IRQ_HANDLED);
}

/* 定时器服务函数 */
void timer_handler(unsigned long arg)
{
    unsigned char value;
    unsigned char num;
    struct irq_keydesc *keydes;
    struct imx6uirq_dev *dev = (struct imx6uirq_dev*)arg;

    num = dev->curkeynum;
    keydes = &dev->irq_keydesc[num];
    value = gpio_get_value(keydes->gpio);
    if (value == 0){
        atomic_set(&dev->keyvalue, keydes->value);
    }
    else
    {
        atomic_set(&dev->keyvalue, 0x80|keydes->value);
        atomic_set(&dev->releasekey, 1);                        /* 标记松开按键 */
    }

    /* 唤醒进程 */
    if (atomic_read(&dev->releasekey)){
        wake_up_interruptible(&dev->r_wait);
    }
}

/* gpio初始化 */
static int keyio_init(void)
{
    int i = 0;
    int ret = 0;

    imx6uirq.nd = of_find_node_by_path("/gpiokey");
    if (imx6uirq.nd == NULL){
        printk("key node not find.\r\n");
        return -EINVAL;
    }

    /* 提取GPIO */
    for (i = 0; i < KEY_NUM; i++){
        imx6uirq.irq_keydesc[i].gpio = of_get_named_gpio(imx6uirq.nd, "key-gpios", i);
        if (imx6uirq.irq_keydesc[i].gpio < 0){
            printk("can't find key%d\r\n", i);
        }
    }

    /* 初始化key所用的IO，并且设置程中断模式 */
    for (i = 0; i < KEY_NUM; i++){
        memset(imx6uirq.irq_keydesc[i].name, 0, sizeof(imx6uirq.irq_keydesc[i].name));
        sprintf(imx6uirq.irq_keydesc[i].name, "KEY%d", i);
        gpio_request(imx6uirq.irq_keydesc[i].gpio, imx6uirq.irq_keydesc[i].name);
        gpio_direction_input(imx6uirq.irq_keydesc[i].gpio);
        imx6uirq.irq_keydesc[i].irqnum = irq_of_parse_and_map(imx6uirq.nd, i);
#if 0
        imx6uirq.irq_keydesc[i].irqnum = gpio_to_irq(imx6uirq.irq_keydesc[i].gpio);
#endif
        printk("key%d:gpio=%d, irqnum=%d\r\n",i, imx6uirq.irq_keydesc[i].gpio, 
                                              imx6uirq.irq_keydesc[i].irqnum);
    }

    /* 申请中断 */
    imx6uirq.irq_keydesc[0].handler = key0_handler;
    imx6uirq.irq_keydesc[0].value = KEY0_VALUE;

    for (i = 0; i < KEY_NUM; i++){
        ret = request_irq(imx6uirq.irq_keydesc[i].irqnum, imx6uirq.irq_keydesc[i].handler,
                          IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, 
                          imx6uirq.irq_keydesc[i].name, &imx6uirq);
        if (ret < 0){
            printk("irq %d request irq fail.\r\n", imx6uirq.irq_keydesc[i].irqnum);
        }
    }

    /* 创建定时器 */
    init_timer(&imx6uirq.timer);
    imx6uirq.timer.function = timer_handler;

    /* 初始化等待队列头 */
    init_waitqueue_head(&imx6uirq.r_wait);
    return 0;
}

/* 打开设备 */
static int imx6uirq_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &imx6uirq;
    return 0;
}

/* 从设备中读取数据 */
static ssize_t imx6uirq_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    int ret = 0;
    unsigned char keyvalue = 0;
    unsigned char releasekey = 0;
    struct imx6uirq_dev* dev = (struct imx6uirq_dev*)filp->private_data;

    if (filp->f_flags & O_NONBLOCK){
        if (atomic_read(&dev->releasekey) == 0){
            return -EAGAIN;
        }else{
            ret = wait_event_interruptible(dev->r_wait, atomic_read(&dev->releasekey));
            if (ret){
                goto wait_error;
            }
        }
    }

    keyvalue = atomic_read(&dev->keyvalue);
    releasekey = atomic_read(&dev->releasekey);

    if (releasekey){
        if (keyvalue & 0x80){
            keyvalue &= ~0x80;
            ret = copy_to_user(buf, &keyvalue, sizeof(keyvalue));
        }else{
            goto data_error;
        }
        atomic_set(&dev->releasekey, 0);
    } else {
        goto data_error;
    }
    return 0;

wait_error:
    return ret;

data_error:
    return -EINVAL;
}

/* poll函数 */
static unsigned int imx6uirq_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    struct imx6uirq_dev *dev = (struct imx6uirq_dev*)filp->private_data;

    poll_wait(filp, &dev->r_wait, wait);

    if (atomic_read(&dev->releasekey)){
        mask = POLLIN | POLLRDNORM;
    }
    return mask;
}

static struct file_operations imx6uirq_fops = {
    .owner  = THIS_MODULE,
    .open   =   imx6uirq_open,
    .read   =   imx6uirq_read,
    .poll   =   imx6uirq_poll,
};

static int __init imx6uirq_init(void)
{
    /* 注册字符设备驱动 */
    if (imx6uirq.major){       /* 已有设备号 */
        imx6uirq.devid = MKDEV(imx6uirq.major, 0);
        register_chrdev_region(imx6uirq.devid, IMX6UIRQ_CNT, IMX6UIRQ_NAME);
    }else{
        alloc_chrdev_region(&imx6uirq.devid, 0, IMX6UIRQ_CNT, IMX6UIRQ_NAME);  /* 申请设备号 */
        imx6uirq.major = MAJOR(imx6uirq.devid);       /* 获得主设备号 */
        imx6uirq.minor = MINOR(imx6uirq.devid);       /* 获得次设备号 */
    }
    printk("imx6uirq major = %d, minor = %d\r\n", imx6uirq.major, imx6uirq.minor);

    /* 初始化cdev */
    imx6uirq.cdev.owner = THIS_MODULE;
    cdev_init(&imx6uirq.cdev, &imx6uirq_fops);

    /* 添加cdev */
    cdev_add(&imx6uirq.cdev, imx6uirq.devid, IMX6UIRQ_CNT);

    /* 创建类 */
    imx6uirq.class = class_create(THIS_MODULE, IMX6UIRQ_NAME);
    if (IS_ERR(imx6uirq.class))
    {
        return PTR_ERR(imx6uirq.class);
    }

    /* 创建设备 */
    imx6uirq.device = device_create(imx6uirq.class, NULL, imx6uirq.devid, NULL, IMX6UIRQ_NAME);

    if (IS_ERR(imx6uirq.device))
    {
        return PTR_ERR(imx6uirq.device);
    }

    /* 初始化定时器 */
    atomic_set(&imx6uirq.releasekey, 0);
    keyio_init();
    return 0;
}

static void __exit imx6uirq_exit(void)
{
    unsigned int i = 0;
    /* 注销定时器 */
    del_timer_sync(&imx6uirq.timer);

    /* 释放中断 */
    for (i = 0; i < KEY_NUM; i++){
        free_irq(imx6uirq.irq_keydesc[i].irqnum, &imx6uirq);
    }
    
    /* 注销字符设备 */
    cdev_del(&imx6uirq.cdev);
    unregister_chrdev_region(imx6uirq.devid, IMX6UIRQ_CNT);

    device_destroy(imx6uirq.class, imx6uirq.devid);
    class_destroy(imx6uirq.class);

    printk("key exit.\n\r");
}

module_init(imx6uirq_init);
module_exit(imx6uirq_exit);

MODULE_LICENSE("GPL");
