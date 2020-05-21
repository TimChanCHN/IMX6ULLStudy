# 1.boot命令
## 1.信息查询命令
1. bdinfo : 查看板子信息
2. printenv(print) : 打印环境变量信息
3. version ：查看u-boot版本信息

## 2.环境变量操作命令
1. 修改环境变量
   1. setenv:如果环境变量之间有空格，环境变量需要用单引号''
   2. saveenv
2. setenv可以用于新建环境变量，操作步骤和其常用用法一直
3. setenv还可以删除环境变量，给环境变量不传参即可

## 3.内存操作指令
> 对于uboot而言，基本上所有数据都是16进制
1. md命令
   1. md.[b, w, l] addr cnt:在起始地址addr处读cnt个数，数字格式由b/w/l决定
2. nm命令
   1. nm.[b, w, l] addr:以指定格式修改地址addr的内容，在？后修改数字，按q退出
3. mm命令
   1. mm.[b, w, l] addr：以指定格式修改地址addr的内容，修改后，地址会以指定的格式递增，按q退出
4. mw命令
   1. mw.[b, w, l] addr value cnt: 以指定格式在起始地址为addr，写上cnt个value数据
5. cp命令
   1. cp.[b, w, l] src des cnt: 以指定的格式把起始地址为src，cnt个数据复制到地址des
6. cmp命令
   1. cmp.[b, w, l] addr1 addr2 cnt: 以指定格式，对起始地址addr1和addr2的cnt个数据进行比较
   

## 4.网络操作指令
1. 需要设置的环境变量  
   1. ipaddr ： 单板地址
   2. ethaddr ： 单板MAC地址
   3. gatewayip ： 单板网关
   4. netmask ： 子网掩码
   5. serverip : 服务器IP，也就是ubuntuIP
2. 单板ping通ubuntu
   1. 如果单板和开发板是通过网线连接，则虚拟机网络设置需要把桥接网卡设置成本地以太网卡
   2. ubuntu设置需要设置成桥接
   3. ubuntu网段需要设置成和单板一致
3. nfs命令
   1. 可以通过nfs命令来下载linux内核
   2. 需要先在ubuntu中安装nfs，并配置
   3. nfs [loadAddress] [[hostIPaddr:]bootfilename--loadaddress是程序要下载到内核的DRAM地址，hostIP是nfs服务器IP，bootfilename是具体待烧写文件的路径
4. tftp命令
   1. 需要先在ubuntu中安装tftp，并配置
   2. 可以通过tftp协议来下载linux内核
   3. tftpboot [loadAddress] [[hostIPaddr:]bootfilename]：意义和nfs一致，但是对于tftp而言，可以直接输入文件名，而不需要输入待烧写文件路径
   
## 5. EMMC和SD卡命令
1. mmc info:查看当前选中的mmc设备信息
2. mmc rescan:扫描当前开发板的所有mmc设备(emmc+sd卡)
3. mmc list：查看当前开发板的所有mmc设备
4. mmc dev [dev] [part]:切换当前mmc设备，[dev]是设备号，[part]是分区号
5. mmc part：查看emmc的分区情况(查看有什么命令)
6. mmc read addr blk #cnt：读取mmc数据到DRAM中，其中，addr是DRAM地址，blk是扇区起始地址，cnt是需要读取的扇区个数
7. mmc write addr blk #cnt:讲数据写到mmc设备中
8. mmc erase blk cnt:擦除扇区内容，[不建议该操作]
> 不能对SD卡/EMMC的前两个扇区进行写，因为里面保存着分区表

## 6.FAT格式文件操作
1. fatinfo <interface> <dev[:part]> : 读取指定MMC设置指定分区的文件系统信息,interface是查询的接口，即mmc
2. fatls <interface> <dev[:part]> [directory]: 查询FAT格式设备的目录和文件信息
3. fstype <interface> <dev>:<part> : 查看 MMC 设备某个分区的文件系统格式
4. fatload <interface> [<dev[:part]> [<addr> [<filename> [bytes [pos]]]]]--将指定的文件读取到 DRAM 中,addr是DRAM地址，filename是要加载到DRAM的文件,bytes是要读取多少字节的数据
5. fatwrite <interface> <dev[:part]> <addr> <filename> <bytes>：将DRAM的数据写到MMC里面，uboot默认不支持fatwrite，需要在指定开发板的配置头文件中添加宏定义`#define CONFIG_FAT_WRITE /* 使能 fatwrite 命令 */`
> 对于EXT格式文件的操作，把上述命令的fat改成ext即可

## 7. boot操作指令
> boot相关指令的本质工作是引导启动Linux
1. bootz [addr [initrd[:size]] [fdt]]:addr 是 Linux 镜像文件在 DRAM 中的位置， initrd 是 initrd 文件在DRAM 中的地址，如果不使用 initrd 的话使用‘-’代替即可， fdt 就是设备树文件在 DRAM 中的地址
2. bootm [addr [initrd[:size]] [fdt]]：addr 是 uImage 在 DRAM 中的首地址， initrd 是 initrd 的地址， fdt 是设备树(.dtb)文件在 DRAM 中的首地址
3. boot:读取环境变量 bootcmd 来启动 Linux 系统
4. bootcmd:在boot的基础上，多执行cmd的命令

## 8.其他命令
1. reset:复位
2. go addr [arg ...]：跳去指定地址执行应用
3. run：运行环境变量中定义的命令
