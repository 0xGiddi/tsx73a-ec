obj-m += qnap8528.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
#CFLAGS_qnap8528.o := -DDEBUG

all:
	$(MAKE) -C $(KERNEL_DIR) M=$$PWD modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$$PWD clean
