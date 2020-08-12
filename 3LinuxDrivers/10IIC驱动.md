# IIC驱动
## 1.Linux IIC驱动框架
1. 在IIC裸机驱动框架中，主要包括IIC主机驱动以及IIC设备驱动，
   1. IIC主机驱动是实现IIC通讯方式的代码，一经编写成功无需根据平台的不同而就该主机驱动
   2. 设备驱动则是提供IIC设备信息，设备驱动通过调用主机驱动的接口函数实现对设备的读写操作
2. Linux的IIC驱动也遵循着设备和驱动分层的思想
   1. IIC总线驱动，就是SOC的IIC控制器驱动，也叫IIC适配器驱动
   2. IIC设备驱动，针对外设IIC设备而编写的驱动
3. IIC总线驱动
   1. 用结构体`struct i2c_adapter`描述，定义在` include/linux/i2c.h `中
   2. 该结构体中有一个重要的成员`algo`，其类型是`const struct i2c_algorithm`，作用是IIC适配器和IIC设备进行通信的方法，定义在`include/linux/i2c.h`
      1. 函数指针`master_xfer`用于完成IIC设备之间的通信
      2. 函数指针`smbus_xfer`是SMBUS总线的传输函数
   3. 向系统注册/注销适配器
      1. 注册--使用动态的总线号:`int i2c_add_adapter(struct i2c_adapter *adapter)`
      2. 注册--使用静态的总线:`int i2c_add_numbered_adapter(struct i2c_adapter *adap)`
      3. 注销:`void i2c_del_adapter(struct i2c_adapter * adap)`
4. IIC设备驱动
   1. 设备信息结构体:`i2c_client`
   2. 设备驱动结构体:`i2c_driver`
      1. 注册驱动设备1:`int i2c_register_driver(struct module *owner, struct i2c_driver *driver)`
      2. 注册驱动设备2：`i2c_add_driver`
      3. 注销:`void i2c_del_driver(struct i2c_driver *driver)`
5. IIC设备和驱动的匹配过程
   1. 同platform设备，有设备树匹配(of_driver_match_device)、ACPI形式匹配(acpi_driver_match_device)、设备名称匹配(i2c_match_id)

## 2.IIC适配器驱动分析
1. 此处主要分析匹配成功后的适配器驱动，官方的IIC驱动在`drivers/i2c/busses/i2c-imx.c`：
   1. 获取中断号:`irq = platform_get_irq(pdev, 0);`
   2. 从设备树中获取I2C1控制器的物理基地址,并对其进行地址映射，从而驱动可以操作对应的寄存器
   3. 给imx_i2c_struct分配空间，而`i2c_adapter`是该结构体的成员变量，同时还给该成员初始化，本质就是给IIC适配器初始化
   4. 注册IIC控制器中断
   5. 设置IIC频率以及I2CR/I2SR寄存器
   6. 往Linux注册i2c_adapter
2. 适配器驱动流程总结：
   1. 初始化i2c_adapter，设置i2c_algorithm，并向内核注册i2c_adapter
   2. 初始化I2C1相关寄存器
3. `i2c_algorithm`有两个成员变量：
   1. functionality：确定该适配器支持的通讯协议
   2. master_xfer：完成于I2C设备通信
      1. 开启I2C通信
      2. 根据上层传递过来的命令，对I2C设备进行读写操作
      3. 停止I2C通信

## 3.IIC设备驱动编写流程
1. 添加设备信息
   1. 不使用设备树：
      1. 给结构体`struct i2c_board_info`的成员`type`/`addr`赋值即可
      2. 也可以在该结构体变量中利用宏`I2C_BOARD_INFO`进行初始化
   2. 使用设备树
      1. 在设备树文件中找到对应的i2c1节点，在里面添加一个子节点，其中compatible和内核驱动的of_match_id匹配，reg是I2C设备地址即可
2. I2C设备数据收发处理
   1. 本质都是利用API函数:`i2c_transfer`
      1. 函数原型:`int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)`
      2. adap:适配器
      3. msgs：消息队列
      4. num:消息数量
   2. 扩展函数:
      1. `int i2c_master_send(const struct i2c_client *client, const char *buf, int count)`
      2. `int i2c_master_recv(const struct i2c_client *client, char *buf, int count)`

