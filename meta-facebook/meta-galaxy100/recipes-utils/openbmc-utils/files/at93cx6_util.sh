#!/bin/sh

source /usr/local/fbpackages/utils/ast-functions

gpio_set I4 1
gpio_set I5 1
gpio_set I6 1
gpio_set I7 1
devmem_clear_bit 0x1e6e2080 18
devmem_clear_bit 0x1e6e2080 19
devmem_clear_bit 0x1e6e208c 13

devmem_clear_bit 0x1e6e2070 5
devmem_clear_bit 0x1e6e2070 12
devmem_clear_bit 0x1e6e2070 13

devmem_set_bit 0x1e780024 2
devmem_set_bit 0x1e780020 2

at93cx6_util.py $@
