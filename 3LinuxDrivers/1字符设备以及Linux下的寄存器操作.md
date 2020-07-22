# 字符设备以及Linux下的寄存器操作
## 1.字符设备
1. 基本概念
   1. 字符设备时Linux驱动三大设备驱动中的一类，对字符设备的操作都是以字节为单位的
   2. Linux中每个设备都有一个设备号，设备号由主设备号和次设备号组成，其中高12位为主设备号，低20位为次设备号，可以通过`cat /proc/devices`查看当前存在的设备情况
   3. 应用程序对字符设备的调用流程
   ```txt
    应用程序：  open, close, read, write...
                    |
    库：          通过调库
                    |
    内核：     通过系统调用进入内核
                    |
    驱动程序：  fops中的open,release,read,write...
                    |
    硬件：        硬件操作
   ```
   4. 模块的加载卸载
      1. insmod/modprobe:前者只是单纯地加载参数模块，后者是连依赖模块一同加载
      2. rmmod/modprobe -r:前者只是单纯地卸载参数模块，后者是连依赖模块一同卸载
      3. lsmod:查看模块
      4. depmod:出现故障`can't open 'modules.dep'`时，可以启动该命令
      5. 加载模块优先用modprobe，卸载模块优先用rmmod
2. 编程过程
   1. 字符设备注册:`register_chrdev`
   2. 定义主设备号、设备名称、文件操作集合fops,对底层硬件的操作集成于fops中
   3. 字符设备注销:`unregister_chrdev`
   4. 该编程过程其实是早期字符设备的编程过程，并不会在/dev目录下生成节点文件，需要利用mknod手动生成节点文件

3. 新字符设备驱动
   1. 背景
      1. 上述的字符设备使用起来十分不方便，首先需要手动通过mknod生成字符设备节点，其次每次注册，都会把所有的次设备号全部使用，还有主设备号需要通过cat /proc/devices来查看，哪些设备号还没使用
      2. 针对以上三点，需要使用一种新的字符设备驱动：自动生成节点文件，次设备号按需使用，主设备号由系统分配
   2. 新字符设备驱动的使用
      1. 获取设备号
         1. 主设备号已定义的情况:
            1. 定义设备号：`MKDEV(major, minor)`
            2. 注册设备号:`register_chrdev_region`
         2. 主设备号未定义
            1. 申请设备号:`alloc_chrdev_region(det_t*, baseminor, count,  *name);`
      2. 注册
         1. 新字符设备的结构体:`struct cdev`
         2. 新字符设备初始化:`cdev_init(*cdev, *fops)`
         3. 向系统添加设备:`cdev_add(*cdev, devid, cnt)`
      3. 自动创建设备节点
         1. 先创建类:`class_create(*owner, *name)`
         2. 创建设备:`device_create(*owner, *parent, devid, *drvdata, *name)`
      4. 退出节点
         1. 删除设备:`device_destroy(*class, devid)`
         2. 删除类:`class_destroy(*class)`
         3. 从系统中删除设备:`cdev_del(*cdev)`
         4. 注销设备号:`unregister_chrdev_region(devid, cnt)`

## 2.Linux下的寄存器操作
1. 基本概念
   1. 内存管理单元(MMU)
      1. 完成虚拟空间到物理空间的映射
      2. 内存保护，设置存储器访问权限，设置虚拟存储空间的缓冲特性
      3. 对于32位的CPU而言，其可以映射的虚拟空间大小是4GB，而真实的物理地址则存储存在内存芯片中
      4. 肯定存在同一个物理地址有多个虚拟地址映射的情况，但改冲突由MMU解决
   2. 内核驱动访问具体地址/寄存器时，不能和裸机程序一样操作，用以下操作:
      1. 需要利用函数`ioremap`，获得一个映射到具体寄存器(对应指定的内存地址)的虚拟内存,改函数利用参数是，映射的物理地址起始地址以及映射空间大小;
      2. 当对该内存/寄存器操作结束后，需要利用函数`iounmap`来释放这个映射关系
      3. 当获取到映射关系后，不能和裸机一样操作的读写操作，同样需要利用函数来对虚拟内存进行操作
         1. 读:readb/readl/readw
         2. 写:writeb/writel/writew
   3. 备注：由于linux系统庞大，一般不直接对寄存器进行操作，一般都是在调试阶段，对寄存器进行监控/操作时，才进行这种操作



