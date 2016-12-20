#!/bin/sh

. /usr/local/bin/openbmc-utils.sh

# select WDTRST
devmem_clear_bit $(scu_addr 2C) 1
devmem_set_bit $(scu_addr 84) 4
