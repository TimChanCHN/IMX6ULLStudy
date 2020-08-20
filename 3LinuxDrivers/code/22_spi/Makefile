KERNELDIR 		:= /home/timchan/linux/imx6ull/kernel/atk_linux
CURRENT_PATH 	:= $(shell pwd)
ROOTFS_DIR 		:= /home/timchan/linux/nfs/rootfs/lib/modules/4.1.15
CROSS_COMPILER 	:= arm-linux-gnueabihf-gcc
MAIN_FILE		:= main
APP_FILE		:= app
COMPILE_OPTION  := -march=armv7-a -mfpu=neon -mfloat-abi=hard
FILE1_NAME 		:= icm20608

obj-m := $(FILE1_NAME).o

build: kernel_modbules
kernel_modbules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules 
	$(CROSS_COMPILER) $(COMPILE_OPTION) $(MAIN_FILE).c -o $(APP_FILE) 

clean: 
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm $(APP_FILE)

cp: 
	cp $(FILE1_NAME).ko $(ROOTFS_DIR)
	cp $(APP_FILE) $(ROOTFS_DIR)

