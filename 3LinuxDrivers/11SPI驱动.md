# SPI驱动
## 1. SPI驱动框架简介
1. SPI驱动框架和I2C驱动框架类似；
2. SPI主机驱动:类似于I2C的适配器驱动，用结构体`spi_master`描述，定义在`include/linux/spi/spi.h`
    1. transfer函数,和i2c_algorithm中的master_xfer函数一样，控制器数据传输函数
    2. transfer_one_message函数，也用于SPI数据发送用于发送一个spi_message,SPI的数据会打包成spi_message,然后以队列方式发送出去
    3. spi_master 申请与释放
        1. 申请:`struct spi_master *spi_alloc_master(struct device *dev, unsigned size)`
        2. 释放:`void spi_master_put(struct spi_master *master)`
    4. spi_master 的注册与注销
        1. 注册:`int spi_register_master(struct spi_master *master)`
        2. 注销:`void spi_unregister_master(struct spi_master *master)`
3. SPI设备驱动:和I2C设备驱动类似，都是由driver、id_table、probe、remove等基本成员组成
    1. 注册与注销spi_driver
        1. 注册:`int spi_register_driver(struct spi_driver *sdrv)`
        2. 注销:`void spi_unregister_driver(struct spi_driver *sdrv)`
   1. SPI主机驱动与设备驱动的匹配过程
      1. 设备树匹配:`of_driver_match_device `
      2. ACPI匹配:`acpi_driver_match_device `
      3. 匹配id_table:`spi_match_id`
      4. 比较name成员变量:`strcmp(spi->modalias, drv->name)`
   
## 2. SPI主机驱动分析
1. 主机驱动和设备驱动的匹配过程和标准平台驱动一样，可通过1.4所述进行匹配；
2. 主机驱动的数据收发流程：
```bash
spi_imx_transfer
   -> spi_imx_pio_transfer
      -> spi_imx_push
         -> spi_imx->tx/rx
```
1. SPI的数据发送接收函数
```c
spi_imx_buf_tx_u8/u16/u32
MXC_SPI_BUF_TX(type)
spi_imx_buf_rx_u8/u16/u32
MXC_SPI_BUF_RX(type)
```

## 3. SPI设备驱动编写流程
1. 设备树信息描述
   1. pinctrl子节点修改
      1. 根据所使用的IO管脚来设置pinctrl子节点
      2. 设置IO管脚的复用功能、输入输出电气特性等，防止有重复使用
   2. SPI设备节点创建与修改
      1. 描述所使用的SPI特性：
      2. 控制的芯片个数、片选信号管脚、状态、引用pinctrl子节点特性
      3. 设备些信息：地址、size、compatible、最大频率等属性
2. SPI设备数据收发处理流程
   1. 重要结构体：
      1. `spi_transfer`:用于描述SPI传输信息，其中成员tx_buf是发送内容，rx_buf是接收内容，len是数据长度(因为spi是全双工，因此发送接收的字节是一样的)
      2. `spi_message`:对spi_transfer组织成spi_message，再统一进行发送接收处理
         1. 使用`spi_message`前需要对其进行初始化:`void spi_message_init(struct spi_message *m)`
         2. 将`spi_transfer`添加倒`spi_message`里面:`void spi_message_add_tail(struct spi_transfer *t, struct spi_message *m)`
         3. 使用同步传输上报spi_message:`int spi_sync(struct spi_device *spi, struct spi_message *message)`
         4. 使用异步传输上报spi_message:`int spi_async(struct spi_device *spi, struct spi_message *message)`
         5. 使用同步传输，会阻塞等待spi_message数据传输完成，使用异步传输，不会阻塞等待spi_message数据传输完成，数据传输完成后，会触发spi_message结构体 中的complete函数