obj-m += tsx73a_ec.o

KERNEL_DIR := /lib/modules/$(shell uname -r)/build
CFLAGS_tsx73a-ec.o := -DDEBUG

all:
	make -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
