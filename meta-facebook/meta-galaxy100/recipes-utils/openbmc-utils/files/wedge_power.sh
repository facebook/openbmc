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
#

. /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

prog="$0"

PWR_SYSTEM_SYSFS="${SYSCPLD_SYSFS_DIR}/pwr_cyc_all_n"
#PWR_USRV_RST_SYSFS="${SCMCPLD_SYSFS_DIR}/come_rst_n"

usage() {
    echo "Usage: $prog <command> [command options]"
    echo
    echo "Commands:"
    echo "  status: Get the current microserver power status"
    echo
    echo "  on: Power on microserver if not powered on already"
    echo "    options:"
    echo "      -f: Re-do power on sequence no matter if microserver has "
    echo "          been powered on or not."
    echo
    echo "  off: Power off microserver ungracefully"
    echo
    echo "  reset: Power reset microserver ungracefully"
    echo "    options:"
    echo "      -s: Power reset whole wedge system ungracefully"
    echo
}

do_status() {
    echo -n "Microserver power is "
    if wedge_is_us_on; then
        echo "on"
    else
        echo "off"
    fi
    return 0
}

do_on_com_e() {
    #echo 1 > $PWR_USRV_SYSFS
    i2cset -f -y 0 0x3e 0x10 0xff 2> /dev/null
    return $?
}

do_on() {
    local force opt ret
    force=0
    while getopts "f" opt; do
        case $opt in
            f)
                force=1
                ;;
            *)
                usage
                exit -1
                ;;

        esac
    done
    echo -n "Power on microserver ..."
    if [ $force -eq 0 ]; then
        # need to check if uS is on or not
        if wedge_is_us_on 10 "."; then
            echo " Already on. Skip!"
            return 1
        fi
    fi

    # power on sequence
    do_on_com_e
    ret=$?
    if [ $ret -eq 0 ]; then
        #enable I2c buffer to EC
        i2cset -f -y 0 0x3e 0x18 0x01 2> /dev/null
        echo " Done"
        logger "Successfully power on micro-server"
    else
        echo " Failed"
        logger "Failed to power on micro-server"
    fi
    return $ret
}

do_off_com_e() {
    #echo 0 > $PWR_USRV_SYSFS
    i2cset -f -y 0 0x3e 0x10 0xfd 2> /dev/null
    sleep 15
    i2cset -f -y 0 0x3e 0x10 0xfe 2> /dev/null
    sleep 10
    return 0
}

do_off() {
    local ret
    echo -n "Power off microserver(about 30s) ..."
    #disable the I2C buffer to EC first
    i2cset -f -y 0 0x3e 0x18 0x07 2> /dev/null
    sleep 1
    do_off_com_e
    ret=$?
    if [ $ret -eq 0 ]; then
        echo " Done"
        logger "Successfully power off micro-server"
    else
        echo " Failed"
        logger "Failed to power off micro-server"
    fi
    return $ret
}

do_reset() {
    local system opt
    system=0
    while getopts "s" opt; do
        case $opt in
            s)
                system=1
                ;;
            *)
                usage
                exit -1
                ;;
        esac
    done
    if [ $system -eq 1 ]; then
        logger "Power reset the whole system ..."
        echo -n "Power reset whole system ..."
        sleep 1
        echo 0 > $PWR_SYSTEM_SYSFS
    else
        if ! wedge_is_us_on; then
            echo "Power resetting microserver that is powered off has no effect."
            echo "Use '$prog on' to power the microserver on"
            return -1
        fi
        echo -n "Power reset microserver ..."
        #echo 0 > $PWR_USRV_RST_SYSFS
        i2cset -f -y 0 0x3e 0x11 0x0 2> /dev/null
        sleep 1
        #echo 1 > $PWR_USRV_RST_SYSFS
        i2cset -f -y 0 0x3e 0x11 0x1 2> /dev/null
        logger "Successfully power reset micro-server"
    fi
    echo " Done"
    return 0
}

do_recovery() {
    local ret
    echo -n "Power on microserver recovery(about 30s) ..."
    come_recovery
    echo " Done"
    logger "Successfully power recover micro-server"
    return 0
}

if [ $# -lt 1 ]; then
    usage
    exit -1
fi

command="$1"
shift

case "$command" in
    status)
        do_status $@
        ;;
    on)
        do_on $@
        ;;
    off)
        do_off $@
        ;;
    reset)
        do_reset $@
        ;;
    recovery)
        do_recovery $@
        ;;
    *)
        usage
        exit -1
        ;;
esac

exit $?
