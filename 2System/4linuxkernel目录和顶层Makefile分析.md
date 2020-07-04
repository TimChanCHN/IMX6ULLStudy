# 4LinuxKernel目录和顶层Makefile分析
## 1.Kernel目录和文件

## 2.顶层Makefile分析
1. make xxx_defconfig/Makefile.build/make过程基本上和Uboot一致。
   1. 在内核的make过程中会生成vmlinux这个ELF格式的可执行文件。它是通过利用shell脚本`scripts/link-vmlinux.s`来连接子目录下的built-in.o,.a文件所生成的可执行文件。
2. vmlinux、 Image， zImage、 uImage 的区别
   1. vmlinux 是编译出来的最原始的内核文件，是未压缩的
   2. Image 是 Linux 内核镜像文件，但是 Image 仅包含可执行的二进制数据。 Image 就是使用 objcopy 取消掉 vmlinux 中的一些其他信息，比如符号表什么的
   3. zImage 是经过 gzip 压缩后的 Image
   4. uImage 是老版本 uboot 专用的镜像文件， uImag 是在 zImage 前面加了一个长度为 64字节的“头”，这个头信息描述了该镜像文件的类型、加载位置、生成时间、大小等信息
   > 备注：一般都是使用经过压缩后的镜像，而zImage和uImage的差别就是头部信息，以适应不同的uboot
