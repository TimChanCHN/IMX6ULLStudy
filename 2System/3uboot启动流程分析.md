# 3. uboot启动流程分析
## 1.u-boot.lds
1. u-boot.lds链接脚本是对u-boot进行`make`之后产生的；
2. u-boot.lds确定了uboot的入口地址`_start`,中断向量表、bss段地址等等

## 2.u-boot的启动流程
1. 开始
   1. 由`u-boot.lds`可知，整个`u-boot`由`_start`开始启动;
   2. `_start`在文件`arch/arm/lib/vectors.S` 中；
   3. `vectors.S`的作用同裸机实验的`start.S`一样，用于初始化中断向量表；
   
2. reset函数
   1. 当u-boot进入vectors.S后，会先执行reset函数。reset函数的最终目的是引导uboot进入main函数。reset函数在`arch/arm/cpu/armv7/start.S`中。
   2. reset函数的作用：
      1. 将CPU设置成SVC模式，并且关闭FIQ和IRQ
      2. 设置中断向量，因为实验中的运行地址不是0x00000000，需要对中断向量地址进行重定向，需要设置VBAR寄存器的Bit13(向量表控制位，为0向量表基地址为 0X00000000，软件可以重定位向量表；为1向量表基地址为 0XFFFF0000，软件不能重定位向量)
      3. 初始化CP15，主要用于关闭MMU(cpu_init_cp15)
      4. 进入lowlevel_init(cpu_init_crit)
   
3. lowlevel_init函数
   1. 该函数处于`arch/arm/cpu/armv7/lowlevel_init.S `
   2. 作用：
      1. 初始化时钟、DDR等
      2. 设置SP堆栈，此时SP指向的内容依然在片内内存中，后续的操作需要把堆栈重定向到DDR中  
    ![lowlevelinit生成的堆栈示意图](https://github.com/TimChanCHN/pictures/raw/master/imx6ul/lowlevel_init.png)

4. s_init函数
   1. 该函数处于`arch/arm/cpu/armv7/mx6/soc.c`
   2. 作用是先判断当前处理器类型，然后返回到函数`cpu_init_crit`  
   ![uboot函数调用路径](https://github.com/TimChanCHN/pictures/raw/master/imx6ul/uboot%E5%87%BD%E6%95%B0%E8%B0%83%E7%94%A8%E8%B7%AF%E5%BE%84.png)

5. _main函数
   1. 该函数处于`arch/arm/lib/crt0.S`
   2. 作用:
      1. 把SP指针指向地址0X0091FF00中，把SP作为参数传进board_init_f_alloc_reserve(common/init/board_init.c)，在该函数中留出早期malloc内存区域和gd(global_data)内存区域;
      2. 在函数board_init_f_init_reserve(`common/init/board_init.c`)对gd区域进行初始化操作；
      3. 调用board_init_f(`common/board_f.c`)函数
      4. 重新设置SP和GD，把SP指向0X9EF44E90，这是DDR地址，即对栈区域进行重定向
      5. 重定位代码，在函数relocate_code中实现，负责将uboot拷贝到新的地方去，此函数定义在`arch/arm/lib/relocate.S`
      6. 重定位中断向量表relocate_vectors，也是定义在`arch/arm/lib/relocate.S`
      7. c_runtime_cpu_setup,关闭I-cache
      8. 清除bss段
      9. 调用函数board_init_r
   
6. board_init_f函数
   1. 位置：`common/board_f.c`
   2. 作用：
      1. 初始化一系列外设，比如串口、定时器，或者打印一些消息等
      2. 初始化 gd 的各个成员变量：uboot会将自己重定位到DRAM的最后的地址区域，目的就是给Linux腾出空间，防止Linux kernel覆盖掉uboot，将DRAM前面区域完整空出来。而结构体gd的作用就是确定uboot应该放到哪个位置、malloc内存池应该放到哪个位置，分配好内存位置和大小
   ![uboot最终内存分布](https://github.com/TimChanCHN/pictures/raw/master/imx6ul/uboot%E6%9C%80%E7%BB%88%E7%9A%84%E5%86%85%E5%AD%98%E5%88%86%E5%B8%83.png)

7. relocate_code函数
   1. 位置: ` arch/arm/lib/relocate.S `
   2. 作用：
      1. 重定位uboot代码，把uboot代码从地址`0X87800000`拷贝到目标地址`r0-r1`
      2. 重定位.rel.dyn(存放.text 段中需要重定位地址的集合)，如果在`arch/arm/config.mk`的line 83加了语句`LDFLAGS_u-boot += -pie`,那就说明采用了位置无关码来解决.rel.dyn段的重定位问题，如果没有的话，则需要对该段数据进行重定向；
   
8. relocate_vectors函数
   1. 位置: ` arch/arm/lib/relocate.S `
   2. 作用：
      1. 设置中断向量表偏移，把新的向量表首地址写入到寄存器VBAR中

9. board_init_r函数
    1.  位置：`common/board_r.c`
    2.  作用：
        1.  通过函数`initcall_run_list`对数组`init_sequence_r`进行初始化,数组`init_sequence_r`存储着对具体操作的初始化
        2.  【问题】：函数`initcall_run_list`并看不出来对函数指针数组`init_sequence_r`各个成员变量的赋值过程，需要查看视频

10. run_main_loop函数
    1.  位置：`common/board_r.c`
    2.  流程
        1.  bootstage_mark_name ： 打印启动信息
        2.  cli_init ：命令初始化
        3.  run_preboot_environment_command ：获取环境变量预启动命令
        4.  bootdelay_process ：读取环境变量 bootdelay 和 bootcmd 的内容，然后将 bootdelay 的值赋值给全局变量 stored_bootdelay
        5.  autoboot_command ：检查倒计时是否结束？倒计时结束之前有没有被打断？如果没有打断，则直接执行`run_command_list`,不然则进入cli_loop，会循环等待处理键盘输入的命令
        6.  run_command_list:该函数用于处理bootcmd里面保存的启动命令(该函数的第一个参数就是bootcmd命令，由函数bootdelay_process获得),而默认的bootcmd的内容，则由对应的开发板头文件(如：mx6ullevk.h)的宏`CONFIG_BOOTCOMMAND`决定。

11. cli_loop函数
    1.  位置: `common/cli.c`
    2.  作用：
        1.  uboot 的命令行处理函数
        2.  最终会通过函数`cmd_process`来处理命令

12. cmd_process函数
    1.  位置：`include/command.h`
    2.  作用：
        1.  执行具体的命令，通过调用宏`U_BOOT_CMD`来对命令进行解析

## 3.u-boot启动内核流程
1. images全局变量
   1. 位置：` cmd/bootm.c `
   2. 作用：
      1. 该类型是bootm_headers_t(其头文件在`include/image.h`)
      2. 该结构体变量包含着内核镜像信息，设备树起始终止地址，initrd起始终止地址，cmdline起始终止地址，以及通过宏来定义内核的不同启动阶段
      3. 全局变量images在uboot中将会被频繁使用

2. do_bootz函数
   1. 位置：`cmd/bootm.c`
   2. 作用：  
      1. 调用函数`bootm_disable_interrupts`关闭函数；
      2. 设置系统镜像为Linux:`images.os.os = IH_OS_LINUX`
      3. 调用函数 `do_bootm_states` 来执行不同的 BOOT 阶段
   
3. bootz_start函数
   1. 位置：`cmd/bootm.c`
   2. 作用：
      1. 调用函数 `do_bootm_states` 来执行不同BOOTM_STATE_START阶段
      2. 设置`images->ep`的值，存储系统镜像入口地址
      3. 通过`bootz_setup`判断系统是否为Linux镜像
      4. 通过`bootm_find_images`查找设备树文件
   
4. do_bootm_states函数
   1. 位置：`common/bootm.c`
   2. 作用：
      1. 通过`states & BOOTM_STATE_XX`来执行相关的启动函数；
      2. 通过函数`bootm_os_get_boot_func`来查找系统启动函数，参数images->os.os为系统类型，根据这个系统类型给`boot_fn`赋值成`do_bootm_linux`
      3. `boot_selected_os`启动Linux内核,最终会调用`boot_fn`函数来启动内核
   
5.  bootm_os_get_boot_func函数
    1.  位置：`common/bootm_os.c`
    2.  作用：
        1.  该函数返回一个`boot_os[os]`,boot_os是一个函数指针数组，根据该函数传递的值，返回该指针数组对应的函数指针
   
6.  do_bootm_linux函数
    1.  位置：`arch/arm/lib/bootm.c`
    2.  作用：
        1.  启动linux的函数
        2.  在`boot_jump_linux`中，若不使用设备树，则会把machine ID传递给内核，否则会利用设备树中“兼容性”的属性来判断内核是否支持该款机器
        3.  通过函数指针kernel_entry获取内核的入口地址；
        4.  

## 4.u-boot的作用
1. 初始化中断向量表，并且设置中断向量可以重定向(设置VBAR寄存器)
2. 设置SP堆栈，把SP指针从片内内存指向DDR/SDRAM(大容量内存)中,并分配SP空间、GD结构体变量等(内存要在此处前进行初始化)
3. 初始化一些外设：定时器、串口
4. 代码重定向，把u-boot往后移动，腾出空间给内核，同时也要重定向中断向量表
5. 判断有无键盘的输入值打断uboot启动，无则进入启动内核，有则进行扫描键入的命令值
6. 启动内核

## 5.移植问题
1. 哪里设置内核运行地址？
   1. uboot的环境变量：loadaddr
   
2. uboot环境变量存储在哪里？
   1. uboot环境变量存储的位置不能和内核和uboot重合，由宏`CONFIG_ENV_OFFSET`决定
   2. 正因为uboot的环境变量存储的位置不和uboot.bin/zImage重合，因此每次掉电，环境变量的内容没有发生改变
   
3. 为什么自启动的时候，uboot无法加载镜像？
   1. 如果内核镜像存储于SDRAM中，由于掉电SDRAM的内容会消失，因此uboot无法加载SDRAM中的镜像
   2. 镜像如果存储于EMMC/SD卡中，当当设备树文件一致，即可以成功加载
   