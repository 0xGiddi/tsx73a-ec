obj-m += tsx73a-ec.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
CFLAGS_tsx73a-ec.o := -DDEBUG

all:
	$(MAKE) -C $(KERNEL_DIR) M=$$PWD modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$$PWD clean
