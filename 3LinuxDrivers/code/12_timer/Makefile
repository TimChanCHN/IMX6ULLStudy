KERNELDIR 		:= /home/timchan/linux/imx6ull/kernel/atk_linux
CURRENT_PATH 	:= $(shell pwd)
ROOTFS_DIR 		:= /home/timchan/linux/nfs/rootfs/lib/modules/4.1.15
CROSS_COMPILER 	:= arm-linux-gnueabihf-gcc
MAIN_FILE		:= main
APP_FILE		:= app
FILE_NAME 		:= timer

obj-m := $(FILE_NAME).o

build: kernel_modbules
kernel_modbules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules 
	$(CROSS_COMPILER) $(MAIN_FILE).c -o $(APP_FILE)

clean: 
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm $(APP_FILE)

cp: 
	cp $(FILE_NAME).ko $(ROOTFS_DIR)
	cp $(APP_FILE) $(ROOTFS_DIR)

