
# Path to the Linux kernel
KDIR := /home/alex/Xilinx_prj/ebaz_linux/buildroot/build/linux-c8b3583bc86352009c6ac61e2ced0e12118f8ebb
export CROSS_COMPILE := /home/alex/Xilinx_prj/ebaz_linux/buildroot/host/bin/arm-buildroot-linux-gnueabihf-
export ARCH := arm
CC := gcc

obj-m := $(TARGET).o

all:
	$(CROSS_COMPILE)$(CC) main.c -o main

clear:
	rm -f *.o
	rm -f eval
