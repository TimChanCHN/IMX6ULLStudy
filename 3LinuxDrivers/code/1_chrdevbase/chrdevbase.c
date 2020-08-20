#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define CHRDEVBASE_MAJOR    200
#define CHRDEVBASE_NAME     "chrdevbase"

static char readbuff[100];
static char writebuff[100];
static char kerneldata[] = "kernel data";

static int chrdevbase_open(struct inode *inode, struct file *filp)
{
    printk("chrdevbase open!\r\n");
    return 0;
}

static int chrdevbase_release(struct inode *inode, struct file *filp)
{
    printk("chrdevbase close!\r\n");
    return 0;
}

static ssize_t chrdevbase_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offset)
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

static ssize_t chrdevbase_write(struct file *filp, const char __user *buff, size_t cnt, loff_t *offset)
{
    int ret = 0;
    ret = copy_from_user(writebuff, buff, cnt);
    if (ret < 0)
    {
        printk("write buff fail!\r\n");
    }
    else
    {
        printk("write from user:%s\r\n", writebuff);
    }
    
    return 0;
}

static struct file_operations chrdevbase_fops = {
    .owner  = THIS_MODULE,
    .open   =   chrdevbase_open,
    .release   =   chrdevbase_release,
    .read   =   chrdevbase_read,
    .write  =   chrdevbase_write,
};

static int __init chrdevbase_init(void)
{
    int retvalue = 0;

    retvalue = register_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME, &chrdevbase_fops);
    if (retvalue < 0)
    {
        printk("chrdevbase register fail...\r\n");
        return -1;
    }
    printk("chrdevbase init.\n\r");
    return 0;
}

static void __exit chrdevbase_exit(void)
{
    unregister_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME);
    printk("chrdevbase exit.\n\r");
}

module_init(chrdevbase_init);
module_exit(chrdevbase_exit);

MODULE_LICENSE("GPL");
