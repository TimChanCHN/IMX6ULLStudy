# input子系统
## 1.简介
1. input子系统就是管理Linux输入的子系统，是Linux内核针对某一类设备而创建的框架；
2. input子系统是由驱动层、核心层、事件处理层组成；
   1. 驱动层：为具体的输入设备的驱动程序，确定输入设备的一些特性等；
   2. 核心层：为驱动层提供输入设备注册和操作接口，并通知事件层对输入事件进行处理
   3. 事件处理层：用于和应用层交互
3. 在编写input子系统时，只需要编写驱动层，对于驱动层不同输入设备，事件层就会有不同的处理，如输入按键，则事件层以按键的特性去处理，或者鼠标，事件层则以相对位置的方式去处理。
   
## 2.input驱动编写流程
1. 注册input_dev
   1. 位置：`include/linux/input.h`
   2. 申请input_dev:`struct input_dev *input_allocate_device(void)`
   3. 注销input_dev:`void input_free_device(struct input_dev *dev)`
   4. 注册input驱动:`int input_register_device(struct input_dev *dev)`
   5. 注销input驱动:`void input_unregister_device(struct input_dev *dev)`
2. 初始化事件
   1. 设置事件类型
      1. `__set_bit(EV_KEY, inputdev->evbit);`
      2. `keyinputdev.inputdev->evbit[0] = BIT_MASK(EV_KEY)`
   2. 设置具体的事件实例：
      1. `__set_bit(KEY_0, inputdev->keybit);`
      2. `keyinputdev.inputdev->keybit[BIT_WORD(KEY_0)] |= BIT_MASK(KEY_0);`
      3. `input_set_capability(keyinputdev.inputdev, EV_KEY, KEY_0);`
3. 上报事件
   1.  input_event：上报指定事件以及对应的值
       1.  `void input_event(struct input_dev *dev, unsigned int type, unsigned int code, int value)`
       2.  dev:input_dev
       3.  type:事件类型
       4.  code:事件示例
       5.  value:事件值
   2. 针对具体的事件，有具体的上报函数，如按键值，有`input_report_key`
   3. 上报结束通知`void input_sync(struct input_dev *dev)`
4. input_event结构体
   1. input_event结构体：应用层可以根据该结构体获取按输入设备的种类、键值等等。

## 3. 其他
1. 利用hexdump查看输入子系统节点获取的信息
2. 命令格式：`hexdump /dev/input/event*`
3. 输出格式
   ```bash
     编号      tv_sec   tv_usec  type code  value
    0000000  5775 0001 bbc9 0001 0001 001c 0001 0000
    #其中type、code、value就是input_event结构体对应的三个成员
    #如上面例子所示，type=0001，说明这是一个EV_KEY事件
    #若type=0000时，则是一个EV_SYN同步事件
   ```