
SRC := $(shell pwd)/drivers
V ?= 1

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) V=$(V)

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) V=$(V) modules_install

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers
