# LCD子系统
## 1.LCD驱动简析
1. 裸机LCD驱动编写流程：
   1. 初始化LCD控制器，重点是配置LCD信息(屏宽、高，hspw,hbp,hfp,vspw,vbp,vfp等)
   2. 初始化LCD像素时钟
   3. 设置RGBLCD显存
   4. 应用程序操作LCD的最根本是操作现存来对LCD进行内容显示
2. 内核LCD注册后在/dev的节点名称是`/dev/fb0`

## 2.LCD驱动编程
1. 如同LCD裸机驱动编程一样，linux下的LCD驱动编程的作用也一样，都是获取LCD屏幕参数、初始化LCD控制器。在Linux内核中，LCD屏幕参数可在设备树中配置，而LCD控制器的驱动编写，内核已经完成。因此当使用不同的LCD屏幕时，只需要配置LCD屏幕参数以及相关IO管脚即可。
2. 内核LCD驱动也叫framebuffer驱动，帧缓存
3. 内核的LCD驱动文件路径在`drivers/video/fbdev/`,在该驱动文件中，`of_device_id`就是存储着设备树的`compatible`信息
4. 每个framebuffer驱动都有一个fb_info结构体，该结构体包含了framebuffer设备的完整属性和操作集合
5. framebuffer驱动的基本工作内容是：
   1. 申请fb_info
   2. 初始化fb_info，在该步工作中实现了LCD屏幕参数的获取
   3. 初始化LCD控制器
   4. 往Linux内核注册初始化好的fb_info
6. LCD驱动程序编写--主要是设备树文件的完善
   1. LCD屏幕IO配置--在.dts文件中的iomuxc节点中，完善`pinctrl_lcdif_dat`,`pinctrl_lcdif_ctrl`,`pinctrl_pwm1`的内容，这三部分分别代表着LCD数据线、控制线、背光PWM引脚配置
   2. LCD 屏幕参数节点信息修改--修改&lcdif
      1. 属性`pinctrl-0`引用iomuxc节点中和LCD管脚相关的属性
      2. 属性`display`引用当前节点下的display子节点
      3. 子节点`display`是LCD的属性信息，
   3. 针对使用的RGB显示模式，可以修改子节点`display`的`bits-per-pixel`
   4. 针对所使用的LCD屏幕，可以在子节点`display`的`native-mode`所引用的子节点中修改LCD屏幕信息
   5. 背光节点信息修改
      1. GPIO复用配置:在节点iomuxc中的节点下`pinctrl_pwm1`
      2. 使用PWM1作为背光调节:在节点`pwm1`的属性`pinctrl-0`下引用iomuxc的`pinctrl_pwm1`,说明使用了该管脚作为PWM1的输出
      3. 设置背光节点`backlight`:设置PWM输出通道、频率、背光等级、默认背光等级等
   
## 3.其他设置
1. 设置LCD作为输出终端：
   1. uboot的`bootargs`中添加`console=tty1`
   2. 在文件`/etc/inittab`中添加`tty1::askfirst:-/bin/sh`，该语句的设置是进入终端前需要按下ENTER键才能进入
2. LCD背光调节
   1. 进入路径`/sys/devices/platform/backlight/backlight/backlight`，用命令`echo`往节点`brightness`输出数字即可以对背光亮度进行调节
3. LCD自动关闭问题
   1. 内核默认在10分钟后LCD自动息屏，可以有下解决方法
      1. 把按键注册成回车键，按下按键亮屏，或者接入键盘按下回车键
      2. 在内核`drivers/tty/vt/vt.c`文件下的`blankinterval`变量初始化为0
      3. 编写应用关闭：打开文件`/dev/tty1`，执行`write(fd, "\033[9;0]", 8);`语句即可