#!/bin/sh

source /usr/local/fbpackages/utils/ast-functions

gpio_set R6 1
gpio_set R7 1
devmem_clear_bit 0x1e6e2080 18
devmem_clear_bit 0x1e6e2080 19
devmem_clear_bit 0x1e6e208c 13
devmem_set_bit 0x1e780024 2
devmem_set_bit 0x1e780020 2
i2cset -f -y 12 0x31 0x14 0xfb
sleep 1
i2cset -f -y 12 0x31 0x14 0xff
bcm5396_util.py $@
