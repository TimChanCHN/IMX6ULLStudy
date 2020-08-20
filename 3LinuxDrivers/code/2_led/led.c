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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define CHRDEVBASE_MAJOR    200
#define CHRDEVBASE_NAME     "led"

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

static struct file_operations chrdevbase_fops = {
    .owner      = THIS_MODULE,
    .open       =   led_open,
    .release    =   led_release,
    .read       =   led_read,
    .write      =   led_write,
};

static int __init led_init(void)
{
    int retvalue = 0;
    u32 val;

    /* 初始化LED */
    /* 1.建立內存映射 */
    IMX6U_CCM_CCGR1     = ioremap(CCM_CCGR1_BASE, 4);
    SW_MUX_GPIO1_IO03   = ioremap(SW_MUX_GPIO1_IO03_BASE, 4);
    SW_PAD_GPIO1_IO03   = ioremap(SW_PAD_GPIO1_IO03_BASE, 4);
    GPIO1_DR            = ioremap(GPIO1_DR_BASE, 4);
    GPIO1_GDIR          = ioremap(GPIO1_GDIR_BASE, 4);

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
    retvalue = register_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME, &chrdevbase_fops);
    if (retvalue < 0)
    {
        printk("led register fail...\r\n");
        return -1;
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

    unregister_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME);
    printk("led exit.\n\r");
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
