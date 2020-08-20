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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define DTSLED_CNT      1                /* 設備個數 */
#define DTSLED_NAME     "dtsled"         /* 名字 */

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
struct dtsled_dev{
    dev_t devid;                /* 设备号 */
    struct cdev cdev;           /* cdev */
    struct class *class;        /* 类 */
    struct device *device;      /* 设备 */
    int major;
    int minor;
    struct device_node *nd;
};

struct dtsled_dev dtsled;

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
    printk("led.ko open!\r\n");
    filp->private_data = &dtsled;        /* 设置私有数据 */
    return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
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
    ret = copy_from_user(databuf, buff, cnt);
    if (ret < 0)
    {
        printk("kernel write fail!\r\n");
        return -EFAULT;
    }

    ledstate = databuf[0];
    led_switch(ledstate);
    
    return 0;
}

static struct file_operations dtsled_fops = {
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

    /* 获取设备数中的属性数据 */
    /* 1. 获取设备节点:alphaled */
    dtsled.nd = of_find_node_by_path("/alphaled");
    if (dtsled.nd == NULL){
        printk("alphaled node can not found.\r\n");
        return -EINVAL;
    }else{
        printk("alphaled node has been found.\r\n");
    }

    /* 2. 获取compatible属性内容 */
    proper = of_find_property(dtsled.nd, "compatible", NULL);
    if (proper == NULL){
        printk("compatible property find failed.\r\n");
    }else{
        printk("compatible = %s\r\n", (char*)proper->value);
    }

    /* 3. 获取status属性内容 */
    ret = of_property_read_string(dtsled.nd, "status", &str);
    if (ret < 0){
        printk("read property fail...\r\n");
    }else{
        printk("status = %s\r\n", str);
    }

    /* 4.获取reg属性内容 */
    ret = of_property_read_u32_array(dtsled.nd, "reg", regdata, 10);
    if (ret < 0){
        printk("reg property read fail.\r\n");
    }else{
        u8 i = 0;
        printk("reg data:\r\n");
        for (i = 0; i < 10; i++)
        {
            printk("%#x ",regdata[i]);
        }
        printk("\r\n");
    }

    /* 1.建立內存映射 */
#if 0
    IMX6U_CCM_CCGR1     = ioremap(regdata[0], regdata[1]);
    SW_MUX_GPIO1_IO03   = ioremap(regdata[2], regdata[3]);
    SW_PAD_GPIO1_IO03   = ioremap(regdata[4], regdata[5]);
    GPIO1_DR            = ioremap(regdata[6], regdata[7]);
    GPIO1_GDIR          = ioremap(regdata[8], regdata[9]);
#else
    IMX6U_CCM_CCGR1     = of_iomap(dtsled.nd, 0);
    SW_MUX_GPIO1_IO03   = of_iomap(dtsled.nd, 1);
    SW_PAD_GPIO1_IO03   = of_iomap(dtsled.nd, 2);
    GPIO1_DR            = of_iomap(dtsled.nd, 3);
    GPIO1_GDIR          = of_iomap(dtsled.nd, 4);
#endif

    /* 2.使能GPIO1時鍾 */
    val = readl(IMX6U_CCM_CCGR1);
    val &= ~(3 << 26);
    val |= (3 << 26);
    writel(val, IMX6U_CCM_CCGR1);

    /* 3.設置復用 */
    writel(5, SW_MUX_GPIO1_IO03);

    /* 寄存器 SW_PAD_GPIO1_IO03 设置 IO 属性 */
    writel(0x10B0, SW_PAD_GPIO1_IO03);

    /* 4.設置GPIO1——IO03爲輸出 */
    val = readl(GPIO1_GDIR);
    val &= ~(1 << 3); 
    val |= (1 << 3);
    writel(val, GPIO1_GDIR);

    /* 5.關閉LED */
    val = readl(GPIO1_DR);
    val |= (1 << 3);
    writel(val, GPIO1_DR);

    /* 注冊字符設備驅動 */
    if (dtsled.major){       /* 已有设备号 */
        dtsled.devid = MKDEV(dtsled.major, 0);
        register_chrdev_region(dtsled.devid, DTSLED_CNT, DTSLED_NAME);
    }else{
        alloc_chrdev_region(&dtsled.devid, 0, DTSLED_CNT, DTSLED_NAME);  /* 申请设备号 */
        dtsled.major = MAJOR(dtsled.devid);       /* 获得主设备号 */
        dtsled.minor = MINOR(dtsled.devid);       /* 获得次设备号 */
    }
    printk("dtsled major = %d, minor = %d\r\n", dtsled.major, dtsled.minor);

    /* 初始化cdev */
    dtsled.cdev.owner = THIS_MODULE;
    cdev_init(&dtsled.cdev, &dtsled_fops);

    /* 添加cdev */
    cdev_add(&dtsled.cdev, dtsled.devid, DTSLED_CNT);

    /* 创建类 */
    dtsled.class = class_create(THIS_MODULE, DTSLED_NAME);
    if (IS_ERR(dtsled.class))
    {
        return PTR_ERR(dtsled.class);
    }

    /* 创建设备 */
    dtsled.device = device_create(dtsled.class, NULL, dtsled.devid, NULL, DTSLED_NAME);

    if (IS_ERR(dtsled.device))
    {
        return PTR_ERR(dtsled.device);
    }

    printk("led init.\n\r");
    return 0;
}

static void __exit led_exit(void)
{
    /* 取消映射 */
    iounmap(IMX6U_CCM_CCGR1);
    iounmap(SW_MUX_GPIO1_IO03);
    iounmap(SW_PAD_GPIO1_IO03);
    iounmap(GPIO1_DR);
    iounmap(GPIO1_GDIR);

    /* 注销字符设备 */
    cdev_del(&dtsled.cdev);
    unregister_chrdev_region(dtsled.devid, DTSLED_CNT);

    device_destroy(dtsled.class, dtsled.devid);
    class_destroy(dtsled.class);

    printk("led exit.\n\r");
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
