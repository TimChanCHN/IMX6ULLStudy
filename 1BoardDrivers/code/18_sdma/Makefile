CROSS_COMPILE ?= arm-linux-gnueabihf-
TARGET		  ?= sdma_uart

CC 				:= $(CROSS_COMPILE)gcc
LD 				:= $(CROSS_COMPILE)ld
OBJCOPY 		:= $(CROSS_COMPILE)objcopy
OBJDUMP			:= $(CROSS_COMPILE)objdump

LIBPATH			:= -lgcc -L /usr/local/arm/gcc-linaro/lib/gcc/arm-linux-gnueabihf/4.9.4

INCUDIRS		:= 	imx6u \
				 	bsp/clk \
					bsp/led \
					bsp/delay \
					bsp/beep \
					bsp/key \
					bsp/gpio \
					bsp/int \
					bsp/exti \
					bsp/epit \
					bsp/keyfilter \
					bsp/uart \
					bsp/lcd \
					bsp/rtc \
					bsp/i2c \
					bsp/ap3216c \
					bsp/spi \
					bsp/icm20608 \
					bsp/ft5426 \
					bsp/backlight \
					bsp/sdma \
					stdio/include \
					utils-tools/malloc \
			
SRCDIRS			:= 	project \
					bsp/clk \
					bsp/led \
					bsp/delay \
					bsp/beep \
					bsp/key \
					bsp/gpio \
					bsp/int \
					bsp/exti \
					bsp/epit \
					bsp/keyfilter \
					bsp/uart \
					bsp/lcd \
					bsp/rtc \
					bsp/i2c \
					bsp/ap3216c \
					bsp/spi \
					bsp/icm20608 \
					bsp/ft5426 \
					bsp/backlight \
					bsp/sdma \
					stdio/lib \
					utils-tools/malloc

INCLUDE 		:= 	$(patsubst %, -I %, $(INCUDIRS))

SFILES			:= $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.S))
CFILES			:= $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.c))

SFILENDIR		:= $(notdir $(SFILES))
CFILENDIR		:= $(notdir $(CFILES))

SOBJS			:= $(patsubst %, obj/%, $(SFILENDIR:.S=.o))
COBJS			:= $(patsubst %, obj/%, $(CFILENDIR:.c=.o))

OBJS			:= $(SOBJS)$(COBJS)

VPATH			:= $(SRCDIRS)

.PHONY:clean

$(TARGET).bin : $(OBJS)
	$(LD) -Timx6u.lds -o $(TARGET).elf $^ $(LIBPATH)
	$(OBJCOPY) -O binary -S $(TARGET).elf $@
	$(OBJDUMP) -D -m arm $(TARGET).elf > $(TARGET).dis
	rm *.imx
	./build_imx $(TARGET).bin

$(SOBJS) : obj/%.o : %.S
	$(CC) -Wall -nostdlib -fno-builtin -c -O2 $(INCLUDE) -o $@ $<

$(COBJS) : obj/%.o : %.c
	$(CC) -march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard -Wall -Wa,-mimplicit-it=thumb -nostdlib -fno-builtin  -c -O2 $(INCLUDE) -o $@ $<

clean:
	rm -rf $(TARGET).elf $(TARGET).bin $(TARGET).dis $(OBJS)

print:
	@echo INCLUDE = $(INCLUDE)
	@echo SFILES = $(SFILES)
	@echo CFILES = $(CFILES)
	@echo SFILENDIR = $(SFILENDIR)
	@echo CFILENDIR = $(CFILENDIR)
	@echo SOBJS = $(SOBJS)
	@echo COBJS = $(COBJS)
	@echo OBJS = $(OBJS)

