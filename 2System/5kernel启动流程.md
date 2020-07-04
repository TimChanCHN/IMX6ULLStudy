# kernel启动流程
## 1.链接脚本 vmlinux.lds
1. 可以通过该链接脚本确定到内核入口在哪
   1. 位置：`arch/arm/kernel/vmlinux.lds`
   2. 内核入口处:`ENTRY(stext)`

## 2.内核启动流程
1. Linux内核入口stext
   1. 位置：`arch/arm/kernel/head.S`
   2. 注意：在内核启动钱需要做 一下工作：
      1. 关闭 MMU
      2. 关闭 D-cache
      3. I-Cache 无所谓
      4. r0=0
      5. r1=machine nr(也就是机器 ID)
      6. r2=atags 或者设备树(dtb)首地址
   3. 函数作用：
      1. 函数 safe_svcmode_maskall:CPU 处于 SVC 模式
      2. 函数__lookup_processor_type:检查当前系统是否支持此CPU,若支持则获取procinfo信息获取成功后把该信息存储到r5中
      3. 函数__create_page_tables:创建页表
      4. __mmap_switched:最终会调用 start_kernel 函数,保存该函数入口地址到r13寄存器中，后续会被调用
      5.  __enable_mmu:使能MMU,该函数会调用__turn_mmu_on来打开MMU，并且最后会执行`__mmap_switched`

2. __mmap_switched函数
   1. 位置:`arch/arm/kernel/head-common.S `
   2. 作用:
      1. 清BSS段，保存设备ID，设备树首地址
      2. 跳去函数`start_kernel`来启动内核
   
3. start_kernel函数
   1. 位置:`init/main.c`
   2. 作用：
      1. lockdep_init:lockdep是死锁检测模块，该函数会初始化两个Hash表，该函数需要尽早执行
      2. set_task_stack_end_magic ：设置任务栈结束魔术数，用于栈溢出检测
      3. smp_setup_processor_id：跟 SMP 有关(多核处理器)，设置处理器 ID
      4. debug_objects_early_init:做一些和 debug 有关的初始化
      5. boot_init_stack_canary:栈溢出检测初始化
      6. cgroup_init_early:cgroup 初始化， cgroup 用于控制 Linux 系统资源
      7. local_irq_disable:关闭当前 CPU 中断
      8. boot_cpu_init:与CPU相关的初始化
      9. page_address_init:页地址相关的初始化
      10. setup_arch(&command_line):架构相关的初始化，此函数会解析传递进来的ATAGS 或者设备树(DTB)文件。会根据设备树里面的 model 和 compatible 这两个属性值来查找Linux 是否支持这个单板。此函数也会获取设备树中 chosen 节点下的bootargs 属性值来得到命令行参数，也就是 uboot 中的 bootargs 环境变量的值，获取到的命令行参数会保存到command_line 中
      11. mm_init_cpumask:与内存相关的初始化
      12. setup_command_line:[疑问]存储命令好参数
      13. setup_nr_cpu_ids:如果只是 SMP(多核 CPU)的话，此函数用于获取CPU核心数量，CPU数量保存在变量nr_cpu_ids中
      14. setup_per_cpu_areas:在 SMP 系统中有用，设置每个 CPU 的 per-cpu 数据 
      15. build_all_zonelists:建立系统内存页区(zone)链表
      16. page_alloc_init:处理用于热插拔 CPU 的页
      17. parse_early_param:解析命令行中的 console 参数
      18. pidhash_init:构建 PID 哈希表,通过构建哈希表可以快速搜索进程信息结构体
      19. vfs_caches_init_early:预先初始化 vfs(虚拟文件系统)的目录项和索引节点缓存
      20. sort_main_extable:定义内核异常列表
      21. trap_init:完成对系统保留中断向量的初始化
      22. mm_init:内存管理初始化
      23. sched_init:初始化调度器，主要是初始化一些结构体
      24. preempt_disable:关闭优先级抢占
      25. rcu_init:初始化 RCU， RCU 全称为 Read Copy Update(读-拷贝修改)
      26. trace_init: 跟踪调试相关初始化
      27. radix_tree_init:基数树相关数据结构初始化 
      28. early_irq_init:初始中断相关初始化,主要是注册 irq_desc 结构体变量，因为 Linux 内核使用 irq_desc 来描述一个中断
      29. init_IRQ:中断初始化 
      30. tick_init:tick 初始化
      31. init_timers:初始化定时器
      32. hrtimers_init:初始化高精度定时器
      33. softirq_init:软中断初始化
      34. time_init:初始化系统时间
      35. local_irq_enable:使能中断
      36. kmem_cache_init_late:slab 初始化， slab 是 Linux 内存分配器
      37. console_init:初始化控制台
      38. locking_selftest:锁自测
      39. kmemleak_init:kmemleak 初始化， kmemleak 用于检查内存泄漏
      40. calibrate_delay:测定 BogoMIPS 值，可以通过 BogoMIPS 来判断 CPU 的性能,BogoMIPS 设置越大，说明 CPU 性能越好
      41. pidmap_init:PID 位图初始化
      42. anon_vma_init:生成 anon_vma slab 缓存
      43. cred_init:为对象的每个用于赋予资格(凭证)
      44. fork_init:初始化一些结构体以使用 fork 函数
      45. proc_caches_init:给各种资源管理结构分配缓存
      46. buffer_init:初始化缓冲缓存
      47. key_init:初始化密钥
      48. security_init:安全相关初始化
      49. vfs_caches_init:为 VFS 创建缓存
      50. signals_init:初始化信号
      51. page_writeback_init:页回写初始化
      52. proc_root_init:注册并挂载 proc 文件系统
      53. cpuset_init:初始化 cpuset， cpuset 是将 CPU 和内存资源以逻辑性和层次性集成的一种机制，是 cgroup 使用的子系统之一
      54. cgroup_init:初始化 cgroup
      55. taskstats_init_early:进程状态初始化
      56. check_bugs: 检查写缓冲一致性
      57. rest_init:rest_init 函数
   
4. rest_init函数
   1. 位置：`init/main.c `
   2. 作用：
      1.  rcu_scheduler_starting:启动 RCU 锁调度器
      2.  利用kernel_thread 创建 kernel_init 进程，该进程也是1号进程(init)
      3.  利用kernel_thread 创建 kthreadd 进程,该进程是2号进程，负责所有内核进程的调度和管理
      4.  调用函数 cpu_startup_entry 来进入 idle 进程
   
5. init进程
    1. 位置：`init/main.c`
    2. 作用：
       1. kernel_init_freeable 函数用于完成 init 进程的一些其他初始化工作
       2. 查找可以使用的init进程，如果ramdisk_execute_command(一个全局变量)存在，则使用根目录下的init程序(可以利用Uboot命令指定:rdinit=xxx)；否则使用ramdisk_execute_command的init进程，在uboot中使用命令init=xxxx指定；如果再不存在，则从`/sbin/init`,`/etc/init`,`/bin/init`,`/bin/sh`中选用，如果该四个目录下依然不存在init进程，则内核启动失败
6. kernel_init_freeable函数
   1. 位置:`init/main.c `
   2. 作用：
      1. do_basic_setup : 用于完成 Linux 下设备驱动初始化工作
      2. 打开设备“/dev/console”,并把标准输入、输出、错误都指定到/dev/console中，通过uboot中的`console=ttymxc0,115200`把设备`/dev/ttymxc0(即串口)`设置成console
      3. 调用函数 prepare_namespace 来挂载根文件系统，通过uboot命令`root=/dev/mmcblk1p2 rootwait rw`指定

