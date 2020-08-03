# platform设备驱动
## 1.Linux驱动分层和分离
1. 如果一个驱动，设备信息和驱动程序耦合在一起，意味着这个驱动更换到另外一个平台上可能会出现不兼容的情况(如一个简单的设备，不同平台的IO调用可能不一样)
2. 针对以上的情况，在Linux这个庞大的系统中，提出了设备与驱动分离的概念：驱动程序和设备程序分别注册到内核中去，对于不同平台，因为是同一类驱动，因此驱动程序可以是一致的，但是设备却不一样，因此不同平台拥有着相同的驱动和自己专有的设备信息，从而达到驱动和设备分离的目的。
3. 驱动程序和设备程序都注册到内核中去，两者就开始匹配工作，如驱动程序找到了自己适配的设备程序，便会匹配成功，开始后续的工作(probe).
4. 在内核中，通过总线bus来管理driver和device
   
## 2.platform平台驱动模型简介
1. platform总线
   1. 用结构体`struct bus_type`管理内核各种总线，定义在`include/linux/device.h`中，platform总线是bus_type的一个实例
   2. platform定义在`drivers/base/platform.c`
   3. match函数：用于比较设备信息和驱动是否匹配，是则说明匹配成功，调用probe函数，有以下几种方法：
      1. of_driver_match_device：从设备树中获取设备信息
      2. acpi_driver_match_device:acpi匹配方式
      3. platform_match_id:用device的name和driver的id_table匹配
      4. 直接匹配dev和dri的name
   
2. platform驱动
   1. 用结构体`struct platform_driver`管理，定义在`include/linux/platform_device.h`中
      1. probe函数：当驱动与设备匹配成功以后 probe 函数就会执行
      2. driver：为device_driver结构体，platform_driver继承了device_driver最基础的驱动框架，又添加了platform自己特有的一些特性，是一种面向对象的编程思维
         1. `struct device_driver`，定义在`include/linux/mod_devicetable.h`
            1. of_device_id--使用设备树时，驱动的匹配列表
               1. compatible:和设备树的compatible属性相匹配
      3. id_table:上面的设备信息和驱动匹配的第三种方法
   2. 注册与注销
      1. 注册:`int platform_driver_register (struct platform_driver *driver)`
      2. 注销:`void platform_driver_unregister(struct platform_driver *drv)`
   
3. platform设备
   1. 使用结构体` platform_device `管理，但如果内核支持设备树，则可以不用设备程序，因为设备信息已经由设备树去描述
      1. name:设备名称，需要和dri的name或id_table中的一项一致
      2. num_resources:资源数量，一般为资源的大小
      3. resources:资源/设备信息，如外设寄存器等
         1. start:资源的起始些信息
         2. end:资源的结束信息
         3. flags:资源的类型
   2. 注册与注销
      1. 注册:`int platform_device_register(struct platform_device *pdev)`
      2. 注销:`void platform_device_unregister(struct platform_device *pdev)`

## 3.设备树下的platform驱动编程
1. 设备树下的platform驱动，就是设备信息由devicetree提供，而驱动端和设备信息的匹配由上一节所提的`of_driver_match_device`来完成；
2. 设备树中节点的`compatible`属性会和`device_driver`的`of_match_table(数据类型是:of_device_id)`成员进行匹配，当两个兼容性一致时，便会执行probe函数
3. 当定义`of_match_table`时，match数据必须预留一个空元素，如下所示:
   ```c
    static const struct of_device_id leds_of_match[] = {
        { .compatible = "atkalpha-gpioled" }, /* 兼容属性 */
        {}
    };
   ```
4. 设备树下的platform编程，就是多了这个match table的定义以及对设备树节点名称、gpio等信息获取，而platform_device的设备信息提供工作则由设备树提供。

## 4. 使用内核自带驱动
1. 内核自带的设备驱动存储在文件夹`/drivers`中，使用内核自带驱动时，需要先通过menuconfig把对应的模块编进内核；
2. 通过对应的c文件，确认设备树节点的compatible,这样设备树才会和内核的匹配成功
3. 编写设备树节点：把设备信息编进设备节点，从而被驱动识别到
