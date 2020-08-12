# RTC驱动
## 1.Linux内核RTC简介
1. RTC设备驱动是一个标准的字符设备驱动；
2. 在Linux内核中，RTC设备用结构体`rtc_device`描述，该结构体定义在`include/linux/rtc.h`;
3. 在结构体`rtc_device`中，有一个操作集合`rtc_class_ops`,该操作集合是最底层的RTC设备操作函数，与文件操作集合`file_operations`不同，文件操作集合是提供给应用层的操作集合
4. 利用RTC设备驱动获取RTC时间或设置RTC时间，由应用层发起，通过文件操作集合的成员函数调用`rtc_class_ops`操作来实现：
   1. [LinuxRTC驱动调用流程]()
5. 注册与注销
   1. 注册：rtc_device_register
   2. 注销: rtc_device_unregister