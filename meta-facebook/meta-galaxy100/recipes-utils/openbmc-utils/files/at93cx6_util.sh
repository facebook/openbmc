#!/bin/sh

source /usr/local/fbpackages/utils/ast-functions

#At this way, it uses to GPIO mode
devmem_clear_bit 0x1e6e2070 12
devmem_clear_bit 0x1e6e2070 13

gpio_export I4
gpio_export I5
gpio_export I6
gpio_export I7

gpio_set E2 1

at93cx6_util.py $@

#resolve to SPI mode for another flash using(backup flash)
devmem_set_bit 0x1e6e2070 12
