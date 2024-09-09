obj-m := sched.o

PWD := $(shell pwd)
KVER := $(shell uname -r)
KDIR := /lib/modules/$(KVER)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD)

clean:
	$(RM) -rf .tmp_versions .*.mk .*.cmd *.ko *.mod.c *.o *.symvers *.order