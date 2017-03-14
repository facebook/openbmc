#!/bin/bash

. /usr/local/fbpackages/utils/ast-functions

WDT2_CTRL_REG=0x1E78502C

# Disable watchdog-2
devmem_clear_bit $WDT2_CTRL_REG 0

exit 0
