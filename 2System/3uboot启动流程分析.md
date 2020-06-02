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
   2. 该函数的作用是设置SP堆栈，此时SP指向的内容依然在片内内存中，后续的操作需要把堆栈重定向到DDR中  
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