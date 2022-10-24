# Top level makefile for building kernel modules
#
# Copyright (c) 2022 Cisco Systems, Inc.
# All rights reserved.
#
SRC := $(shell pwd)/git/drivers
V ?= 1

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) V=$(V)

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) V=$(V) modules_install

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers
