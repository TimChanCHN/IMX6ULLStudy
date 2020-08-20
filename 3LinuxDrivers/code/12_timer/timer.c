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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/* 
    实验目的:当按键按下时，启动定时器，过了一段时间，若按键依然按下，则认为按键按下
 */

#define TIMER_CNT      1                /* 設備個數 */
#define TIMER_NAME     "timer"         /* 名字 */
#define CLOSE_CMD       (_IO(0XEF, 0X01))
#define OPEN_CMD        (_IO(0XEF, 0X02))
#define SETPERIOD_CMD   (_IO(0XEF, 0X03))

#define LED_ON          1
#define LED_OFF         0


/* newchrdev设备结构体 */
struct timer_dev{
    dev_t devid;                /* 设备号 */
    struct cdev cdev;           /* cdev */
    struct class *class;        /* 类 */
    struct device *device;      /* 设备 */
    int major;
    int minor;
    struct device_node *nd;
    int led_gpio;       
    int timeperiod;                 /* 定时周期 */
    struct timer_list timer;    /* 定时器 */
    spinlock_t lock;              /* 原子变量 */
};

struct timer_dev timerdev;

static int led_init(void)
{
    int ret = 0;

    timerdev.nd = of_find_node_by_path("/gpioled");
    if (timerdev.nd == NULL){
        return -EINVAL;
    }

    timerdev.led_gpio = of_get_named_gpio(timerdev.nd, "led-gpio", 0);
    if (timerdev.led_gpio < 0){
        printk("can't get led\r\n");
        return -EINVAL;
    }

    /* 申请GPIO资源 */
    gpio_request(timerdev.led_gpio, "led");
    ret = gpio_direction_output(timerdev.led_gpio, 1);
    if (ret < 0){
        printk("can't set gpio\r\n");
    }
    return 0;
}

static int timer_open(struct inode *inode, struct file *filp)
{
    int ret;
    filp->private_data = &timerdev;        /* 设置私有数据 */

    timerdev.timeperiod = 1000;

    ret = led_init();
    if (ret < 0){
        return ret;
    }
    return 0;
}

static long timer_unlock_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
    struct timer_dev *dev = (struct timer_dev*)fp->private_data;
    int timerperiod;
    unsigned long flags;

    switch(cmd){
        case CLOSE_CMD:
            del_timer_sync(&timerdev.timer);
            break;
        
        case OPEN_CMD:
            spin_lock_irqsave(&dev->lock, flags);
            timerperiod = dev->timeperiod;
            spin_unlock_irqrestore(&dev->lock, flags);
            mod_timer(&dev->timer, jiffies+msecs_to_jiffies(timerperiod));
            break;

        case SETPERIOD_CMD:
            spin_lock_irqsave(&dev->lock, flags);
            dev->timeperiod = arg;
            spin_unlock_irqrestore(&dev->lock, flags);
            mod_timer(&dev->timer, jiffies+msecs_to_jiffies(timerperiod));
            break;
        
        default:
            break;
    }
    return 0;
}

static struct file_operations timerdev_fops = {
    .owner          = THIS_MODULE,
    .open           =   timer_open,
    .unlocked_ioctl =   timer_unlock_ioctl,
};

void timer_handler(unsigned long arg)
{
    struct timer_dev *dev = (struct timer_dev*)arg;
    static int sta = 1;
    int timeperiod;
    unsigned long flags;

    sta = !sta;
    gpio_set_value(dev->led_gpio, sta);
    spin_lock_irqsave(&dev->lock, flags);
    timeperiod = dev->timeperiod;
    spin_unlock_irqrestore(&dev->lock, flags);
    mod_timer(&dev->timer, jiffies+msecs_to_jiffies(timeperiod));
}

static int __init timerdev_init(void)
{
    /* 注册字符设备驱动 */
    if (timerdev.major){       /* 已有设备号 */
        timerdev.devid = MKDEV(timerdev.major, 0);
        register_chrdev_region(timerdev.devid, TIMER_CNT, TIMER_NAME);
    }else{
        alloc_chrdev_region(&timerdev.devid, 0, TIMER_CNT, TIMER_NAME);  /* 申请设备号 */
        timerdev.major = MAJOR(timerdev.devid);       /* 获得主设备号 */
        timerdev.minor = MINOR(timerdev.devid);       /* 获得次设备号 */
    }
    printk("timerdev major = %d, minor = %d\r\n", timerdev.major, timerdev.minor);

    /* 初始化cdev */
    timerdev.cdev.owner = THIS_MODULE;
    cdev_init(&timerdev.cdev, &timerdev_fops);

    /* 添加cdev */
    cdev_add(&timerdev.cdev, timerdev.devid, TIMER_CNT);

    /* 创建类 */
    timerdev.class = class_create(THIS_MODULE, TIMER_NAME);
    if (IS_ERR(timerdev.class))
    {
        return PTR_ERR(timerdev.class);
    }

    /* 创建设备 */
    timerdev.device = device_create(timerdev.class, NULL, timerdev.devid, NULL, TIMER_NAME);

    if (IS_ERR(timerdev.device))
    {
        return PTR_ERR(timerdev.device);
    }

    /* 初始化定时器 */
    init_timer(&timerdev.timer);
    timerdev.timer.function = timer_handler;
    timerdev.timer.data = (unsigned long)&timerdev;
    printk("timer init.\n\r");
    return 0;
}

static void __exit timerdev_exit(void)
{
    /* 注销定时器 */
    del_timer_sync(&timerdev.timer);
    /* 注销字符设备 */
    cdev_del(&timerdev.cdev);
    unregister_chrdev_region(timerdev.devid, TIMER_CNT);

    device_destroy(timerdev.class, timerdev.devid);
    class_destroy(timerdev.class);

    printk("key exit.\n\r");
}

module_init(timerdev_init);
module_exit(timerdev_exit);

MODULE_LICENSE("GPL");
