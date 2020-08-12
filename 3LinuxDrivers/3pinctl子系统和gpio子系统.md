# pinctrl子系统和gpio子系统
## 1.pinctrl子系统
1. 顾名思义，pinctrl为管脚配置子系统，对于管脚的配置，一般有IO复用、电气特性、输出特性等设置
2. 在设备树中设置了相应pin脚的配置信息，当调用pinctrl子系统驱动时，先会匹配兼容性，之后再获取设备树中的参数，当匹配成功后，则把设备树配置写进对应的寄存器中(pinctrl中写好)
3. 例子
   1. 节点名字前缀必须是pinctrl
   2. 添加属性"fsl,pins"
   3. 属性内容是:`fsl,pins=<MX6UL_PAD_UART1_RTS_B__GPIO1_IO19 0x17059>`
      1. 宏`MX6UL_PAD_UART1_RTS_B__GPIO1_IO19 = 0x0090 0x031C 0x0000 0x5 0x0`，对于该宏，各个参数的意义是:`<mux_reg conf_reg input_reg mux_mode input_val>`--该宏可以从`arch/arm/boot/dts/imx6ul-pinfunc.h`中获取
      2. mux_reg:设置复用的寄存器偏移量
      3. conf_reg:设置电气特性寄存器偏移量
      4. input_reg:设置输入寄存器偏移量
      5. mux_mode:复用寄存器的值
      6. input_val:输入寄存器的值
      7. 宏里面并没有conf_reg的值，但是该值存储到该属性的第六个`0x17059`
      8. 因为对于一个pin脚的使用，复用寄存器、输入寄存器、电气特性配置寄存器的偏移地址固定，而复用寄存器、输入寄存器的值也相对固定，所以用宏来归纳，但是电气特性(设置pin频率、上下拉等)使用工况相对多种，因此不归纳到宏里面
      9. `fsl,pins`中，`,`两边不能有空格，否则语法错误

## 2.gpio子系统
1. 当pinctrl子系统把管脚复用为gpio功能时，就可以用gpio子系统来设置gpio的特性
2. 当设备树设置好之后，就可以使用gpio子系统提供的API函数来操作指定的GPIO
   1. 申请GPIO管脚:`int gpio_request(unsigned gpio, const char *label)`
   2. 释放GPIO管脚:`void gpio_free(unsigned gpio)`
   3. 设置GPIO为输入:`int gpio_direction_input(unsigned gpio)`
   4. 设置GPIO为输出:`int gpio_direction_output(unsigned gpio, int value)`
   5. 获取GPIO的值:`gpio_get_value(unsigned gpio)`
   6. 设置GPIO的值:`gpio_set_value(unsigned gpio)`
3. 设备树中GPIO节点的操作流程
   1. 添加pinctrl信息 
      1. 添加pinctrl-names属性:`pinctrl-names = "default";` 
      2. 添加pinctrl-0节点(主要是引用pinctrl节点):`pinctrl-0 = <&pinctrl_test>;`
   2. 添加GPIO属性信息:
      1. `gpio = <&gpio1 0 GPIO_ACTIVE_LOW>`
      2. 意思是使用GPIO1组，第0个GPIO，低电平有效
4. 如果开发设备树文件是在别人文件的基础上进行修改的话，由于开发板在运行的时候只能会有一个功能，因此同一个GPIO同样功能不能被多个节点调用需要检查两个方面
   1. pinctrl配置，查看是否有多个pinctrl节点在调用
      1. 在pinctrl节点，查找GPIO配置的节点，在当前设备树文件中查找即可
   2. 若PIN设置为GPIO的时候，查看该GPIO有没被多个外设调用
      1. 查看GPIO属性，如`led-gpio = <&gpio1 3 GPIO_ACTIVE_LOW>;`,在当前文件夹查找，是否有多个`gpio1 3`即可 

## 3.pinctrl子系统和gpio子系统搭配使用
1. 设备树增加pinctrl节点，用于确定GPIO的复用、电气特性等；
2. gpio节点中，通过引用pinctrl节点，进而设置GPIO管脚的ACTIVE特性；
3. 当内核启动后，pinctrl则会加载到内核中，而pinctrl的配置会被pinctrl子系统加载到内核中，gpio的配置会被gpio子系统加载到内核中。