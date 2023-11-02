#!/bin/bash

# Read AST2620 register SCU014 [23:16] to retrieve the board hardware revision.

SCU014_REG=0x1e6e2014
HW_REV_BIT=16
BOARD_REV=$(($(($(devmem "${SCU014_REG}") >> "${HW_REV_BIT}")) & 0xff))
echo "BMC Board Revision: A${BOARD_REV}"
