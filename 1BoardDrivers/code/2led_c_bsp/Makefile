CROSS_COMPILE 	?= arm-linux-gnueabihf-									# 变量定义，?=指如果之前没有定义该变量，则使用该变量定义
TARGET		  	?= bsp

CC 				:= $(CROSS_COMPILE)gcc
LD				:= $(CROSS_COMPILE)ld
OBJCOPY 		:= $(CROSS_COMPILE)objcopy
OBJDUMP 		:= $(CROSS_COMPILE)objdump

INCDIRS 		:= imx6ul \
				   bsp/clk \
				   bsp/led \
				   bsp/delay 
				   			   
SRCDIRS			:= project \
				   bsp/clk \
				   bsp/led \
				   bsp/delay 
				   
				   
INCLUDE			:= $(patsubst %, -I %, $(INCDIRS))				# 对$(INCDIRS)的所有内容，都添加"-I "

SFILES			:= $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.S))		# 把$(SRCDIRS)取一个值送到dir,再参与运算$(wildcard $(dir)/*.S)
																			# 目的时获取$(SRCDIRS)所有目录下的.S文件
CFILES			:= $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.c))

SFILENDIR		:= $(notdir  $(SFILES))							# 获取$(SFILES)下的所有文件名，去除路径前缀
CFILENDIR		:= $(notdir  $(CFILES))

SOBJS			:= $(patsubst %, obj/%, $(SFILENDIR:.S=.o))
COBJS			:= $(patsubst %, obj/%, $(CFILENDIR:.c=.o))
OBJS			:= $(SOBJS) $(COBJS)

VPATH			:= $(SRCDIRS)

.PHONY: clean													# 伪目标，如果不加该关键词，如果当前目录下有clean文件，则不会执行
																# 加了.PHONY,则会继续执行该目标
	
$(TARGET).bin : $(OBJS)
	$(LD) -Timx6ul.lds -o $(TARGET).elf $^
	$(OBJCOPY) -O binary -S $(TARGET).elf $@
	$(OBJDUMP) -D -m arm $(TARGET).elf > $(TARGET).dis

$(SOBJS) : obj/%.o : %.S										# 静态模式，依赖是参数%.s，编译后的结果存放到obj目录下
	$(CC) -Wall -nostdlib -c -O2  $(INCLUDE) -o $@ $<

$(COBJS) : obj/%.o : %.c
	$(CC) -Wall -nostdlib -c -O2  $(INCLUDE) -o $@ $<
	
clean:
	rm -rf $(TARGET).elf $(TARGET).dis $(TARGET).bin $(COBJS) $(SOBJS)

	
