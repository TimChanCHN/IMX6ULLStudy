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
#include <linux/input.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/* 
    实验目的:当按键按下时，启动定时器，过了一段时间，若按键依然按下，则认为按键按下
 */

#define KEYINPUT_CNT      1                /* 設備個數 */
#define KEYINPUT_NAME     "imx6u-irq"         /* 名字 */

#define KEY0_VALUE        0X01
#define INVAKEY           0XFF
#define KEY_NUM           1

/* 中断irq描述结构体 */
struct irq_keydesc{
    int gpio;
    int irqnum;
    unsigned char value;        // 键值
    char name[10];
    irqreturn_t (*handler)(int, void*);
};

/* newchrdev设备结构体 */
struct keyinput_dev{
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
    struct input_dev *inputdev;                     /* input结构体 */
};

struct keyinput_dev keyinput;

/* 按键0中断服务函数 */
static irqreturn_t key0_handler(int irq, void* dev_id)
{
    struct keyinput_dev* dev = (struct keyinput_dev*)dev_id;

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
    struct keyinput_dev *dev = (struct keyinput_dev*)arg;

    num = dev->curkeynum;
    keydes = &dev->irq_keydesc[num];
    value = gpio_get_value(keydes->gpio);
    if (value == 0){
        input_report_key(dev->inputdev, keydes->value, 1);
        input_sync(dev->inputdev);
    }
    else
    {
        input_report_key(dev->inputdev, keydes->value, 0);
        input_sync(dev->inputdev);    
    }
}

/* gpio初始化 */
static int keyio_init(void)
{
    int i = 0;
    int ret = 0;

    keyinput.nd = of_find_node_by_path("/gpiokey");
    if (keyinput.nd == NULL){
        printk("key node not find.\r\n");
        return -EINVAL;
    }

    /* 提取GPIO */
    for (i = 0; i < KEY_NUM; i++){
        keyinput.irq_keydesc[i].gpio = of_get_named_gpio(keyinput.nd, "key-gpios", i);
        if (keyinput.irq_keydesc[i].gpio < 0){
            printk("can't find key%d\r\n", i);
        }
    }

    /* 初始化key所用的IO，并且设置程中断模式 */
    for (i = 0; i < KEY_NUM; i++){
        memset(keyinput.irq_keydesc[i].name, 0, sizeof(keyinput.irq_keydesc[i].name));
        sprintf(keyinput.irq_keydesc[i].name, "KEY%d", i);
        gpio_request(keyinput.irq_keydesc[i].gpio, keyinput.irq_keydesc[i].name);
        gpio_direction_input(keyinput.irq_keydesc[i].gpio);
        keyinput.irq_keydesc[i].irqnum = irq_of_parse_and_map(keyinput.nd, i);
#if 0
        keyinput.irq_keydesc[i].irqnum = gpio_to_irq(keyinput.irq_keydesc[i].gpio);
#endif
        printk("key%d:gpio=%d, irqnum=%d\r\n",i, keyinput.irq_keydesc[i].gpio, 
                                              keyinput.irq_keydesc[i].irqnum);
    }

    /* 申请中断 */
    keyinput.irq_keydesc[0].handler = key0_handler;
    keyinput.irq_keydesc[0].value = KEY_0;

    for (i = 0; i < KEY_NUM; i++){
        ret = request_irq(keyinput.irq_keydesc[i].irqnum, keyinput.irq_keydesc[i].handler,
                          IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, 
                          keyinput.irq_keydesc[i].name, &keyinput);
        if (ret < 0){
            printk("irq %d request irq fail.\r\n", keyinput.irq_keydesc[i].irqnum);
            return -EFAULT;
        }
    }

    /* 创建定时器 */
    init_timer(&keyinput.timer);
    keyinput.timer.function = timer_handler;

    /* 申请inputdev */
    keyinput.inputdev = input_allocate_device();
    keyinput.inputdev->name = KEYINPUT_NAME;

#if 0
    /* 初始化input事件 */
    __set_bit(EV_KEY, keyinput.inputdev->evbit);        /* 按键事件 */
    __set_bit(EV_REP, keyinput.inputdev->evbit);        /* 重复时间 */

    /* 设置产生哪些按键 */
    __set_bit(KEY_0, keyinput.inputdev->keybit);
#endif

#if 0 
    keyinput.inputdev->evbit[0] = BIT_MASK(EV_REP) | BIT_MASK(EV_KEY);
    keyinput.inputdev->keybit[BIT_WORD(KEY_0)] |= BIT_MASK(KEY_0);
#endif

    keyinput.inputdev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);
    input_set_capability(keyinput.inputdev, EV_KEY, KEY_0);

    /* 注册输入设备 */
    ret = input_register_device(keyinput.inputdev);
    if (ret){
        printk("register input device failed.\r\n");
        return ret;
    }
    return 0;
}


static int __init keyinput_init(void)
{
    keyio_init();
    return 0;
}

static void __exit keyinput_exit(void)
{
    unsigned int i = 0;
    /* 注销定时器 */
    del_timer_sync(&keyinput.timer);

    /* 释放中断 */
    for (i = 0; i < KEY_NUM; i++){
        free_irq(keyinput.irq_keydesc[i].irqnum, &keyinput);
    }
    
    /* 释放input_dev */
    input_unregister_device(keyinput.inputdev);
    input_free_device(keyinput.inputdev);

    printk("key exit.\n\r");
}

module_init(keyinput_init);
module_exit(keyinput_exit);

MODULE_LICENSE("GPL");
