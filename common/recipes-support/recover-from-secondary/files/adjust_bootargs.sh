#!/bin/bash
# Copyright 2014-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

# Sets up the bootargs so that we can boot with the desired chips
# exposed.

set -xeu -o pipefail

if grep -q wedge100 /etc/issue
then
    openbmc_gpio_util.py config ROMCS1#
fi

# If verified boot mount flash1rw as well
if [ -f /usr/local/bin/vboot-util ]
then
    if grep -q rom /proc/mtd
    then
        echo "Verified boot"
        EXTRA_BOOTARGS="mtdparts=spi0.0:32M@0x0(flash0),0x20000@0x60000(env);spi0.1:32M@0x0(flash1),0x01ff0000@0x10000(flash1rw) dual_flash=1"
    else
        echo "Verified boot detected, but no ROM found"
        exit 1
    fi
else
    echo "Non-verified boot"
    EXTRA_BOOTARGS="mtdparts=spi0.0:32M@0x0(flash0),0x20000@0x60000(env);spi0.1:-(flash1) dual_flash=1";
fi

bootargs=$(fw_printenv bootargs|sed 's/bootargs=//')
if grep -q mtdparts= <<<"$bootargs"
then
    echo "Bootargs already applied."
    exit 0
fi

bootargs="${bootargs} ${EXTRA_BOOTARGS}"

fw_setenv bootargs "${bootargs}"

echo "All set. Now go ahead and reboot!"
