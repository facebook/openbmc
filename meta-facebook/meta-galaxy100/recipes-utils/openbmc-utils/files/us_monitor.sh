#!/bin/bash
#
# Copyright 2015-present Facebook. All Rights Reserved.
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

. /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

# Because the voltage leak from uS COM pins could cause uS to struck when
# transitting from S5 to S0, we will need to explicitely pull down uS COM
# pins before powering off/reset and restoring COM pins after

TH_SHUTDOWN_VOL=949
TH_RESTORE_VOL=1028
th_shutdown=0

restore_us_com() {
    repeater_config
    KR10G_repeater_config
    wedge_power_on_board
    i2cset -f -y 0 0x3e 0x10 0xff 2> /dev/null
    logger "Power on micro-server after inserting SCM"
}

lm57_monitor() {
    vol=$(cat /sys/devices/platform/ast_adc.0/in0_input 2> /dev/null)
    vol=$(($vol / 2))
    if [ $vol -le ${TH_SHUTDOWN_VOL} ] && [ $th_shutdown -eq 0 ] ; then
        echo "Shut down TH power by LM57 over temperature..."
        echo "ADC=$vol, standard=$TH_SHUTDOWN_VOL"
        wedge_power_off_board
        th_shutdown=1
        logger "Power off micro-server due to LM57 over temperature (${vol} vs ${TH_SHUTDOWN_VOL})"
    fi
    if [ $vol -ge ${TH_RESTORE_VOL} ] && [ $th_shutdown -eq 1 ] ; then
        echo "restore TH power by LM57 temperature drop..."
        echo "ADC=$vol, standard=$TH_RESTORE_VOL"
        wedge_power_on_board
        th_shutdown=0
        logger "Power on micro-server after LM57 temperature drop (${vol} vs ${TH_SHUTDOWN_VOL})"
    fi
    cpld_val=$(i2cget -f -y 12 0x31 0x15 2> /dev/null)
    if [ $vol -ge ${TH_RESTORE_VOL} ] && [ "$cpld_val" = "0x11" ] ; then
        echo "restore TH power which shutdown by CPLD..."
        i2cset -f -y 12 0x31 0x15 0x0 2> /dev/null
        wedge_power_on_board
        th_shutdown=0
        logger "Power on micro-server caused by CPLD (${cpld_val}, ${vol} vs ${TH_SHUTDOWN_VOL})"
    fi
}

val=0
ret=0
echo 1 > /sys/devices/platform/ast_adc.0/adc0_en
((card_val=$(i2cget -f -y 12 0x31 0x0 2> /dev/null | head -n 1)))
card_val=$(($card_val % 16))
if [ $card_val -le 1 ]; then
    #EVT or DVT1 board
    TH_SHUTDOWN_VOL=1296
    TH_RESTORE_VOL=1402
fi
while true; do
    galaxy100_scm_is_present
    ret=$?
    if [ $ret -eq 0 ] && [ $val -eq 1 ]; then
        echo "SCM is inserted, Power ON!"
        sleep 15
        restore_us_com
    fi
    val=$ret
    lm57_monitor
    usleep 500000
done
