#!/bin/sh
#
# Copyright 2019-present Facebook. All Rights Reserved.
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
#

source /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

SFP_AUTO_DETECT="${SCMCPLD_SYSFS_DIR}/sfp_auto_detect"
COM_E_PWRGD=`head -n 1 ${SCMCPLD_SYSFS_DIR}/pwrgd_pch_pwrok`

usage(){
    program=`basename "$0"`
    echo "Usage:"
    echo "     $program led/combo/all"
}

if [ $# -ne 1 ]; then
    usage
    exit -1
fi

# When COM-e power is not ready, it doesn't work to set PHY register.
# Therefore, waiting for COM-e power ready, then start to set PHY register.
while [ "$COM_E_PWRGD" == "0x0" ];
do
    COM_E_PWRGD=`head -n 1 ${SCMCPLD_SYSFS_DIR}/pwrgd_pch_pwrok`
    sleep 5
done

devmem_set_bit $(scu_addr 88) 30
devmem_set_bit $(scu_addr 88) 31

if [ "$1" == "led" ]; then
    echo "Wait a few seconds to setup management port LED..."
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x01d
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x3453
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x012
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0xa03
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x019
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x2418
    echo "Done!"
elif [ "$1" == "combo" ]; then
    echo "Wait a few minutes to setup management combo port..."
    ast-mdio.py --mac 1 --phy 0x9 write 0x17 0x0f7e
    ast-mdio.py --mac 1 --phy 0x9 write 0x15 0x00
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x083c
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x00
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x2f
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x7167
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x21
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x7c05
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x00
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x1140
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x21
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x7c04
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x09
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x0200
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x04
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x01e1
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x00
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x1340
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x234
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0xd18f
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0xb00
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x1140
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x23e
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x7ae2
    echo 1 > $SFP_AUTO_DETECT
    echo "Done!"
elif [ "$1" == "all" ]; then
    echo "Wait a few minutes to setup management port..."
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x01d
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x3453
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x012
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0xa03
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x019
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x2418
    ast-mdio.py --mac 1 --phy 0x9 write 0x17 0x0f7e
    ast-mdio.py --mac 1 --phy 0x9 write 0x15 0x00
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x083c
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x00
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x2f
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x7167
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x21
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x7c05
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x00
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x1140
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x21
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x7c04
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x09
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x0200
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x04
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x01e1
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x00
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x1340
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x234
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0xd18f
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0xb00
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x1140
    ast-mdio.py --mac 1 --phy 0x9 write 0x1e 0x23e
    ast-mdio.py --mac 1 --phy 0x9 write 0x1f 0x7ae2
    echo 1 > $SFP_AUTO_DETECT
else
    usage
    exit -1
fi
