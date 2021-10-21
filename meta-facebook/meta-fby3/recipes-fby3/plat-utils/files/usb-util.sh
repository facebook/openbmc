#!/bin/sh
#
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

. /usr/local/fbpackages/utils/ast-functions

cmd=$1
fru=$2
dev=$3
busid=
exp=
retry=10

function show_usage() {
  echo "Usage: usb-util.sh [ bind | unbind ] [ slot1-2U | slot1-2U-exp | slot1-2U-top | slot1-2U-bot ] [ pesw-uart, uart0, uart1, uart2, dev0, dev1, ..., dev11 ]"
  echo "Usage: usb-util.sh bus-id [ slot1-2U | slot1-2U-exp | slot1-2U-top | slot1-2U-bot ] [ pesw-uart, uart0, uart1, uart2, dev0, dev1, ..., dev11 ]"
}

function check_file_exist() {
    local found=0
    for i in $(seq 0 $retry)
    do
        ls /sys/bus/usb/devices/ | grep -x $busid
        if [ $? -eq 0 ]; then
        # if usb path exist
            found=1
            break
        fi
        sleep 2
    done

    if [ $found -eq 0 ]; then
        echo "bus id: $busid not found"
        exit -1
    fi
}

function get_2u_gpv3_bus_id() {
    if [ $exp != "0x00" ] && [ $exp != "0x03" ]; then
        echo "command not supported by this platform"
        exit -1
    fi

    case $dev in
    pesw-uart)
        busid="1-1.3.4.2.4"
    ;;

    uart0)
        busid="1-1.3.1"
    ;;

    uart1)
        busid="1-1.3.2.2"
    ;;

    uart2)
        busid="1-1.3.2.4"
    ;;

    dev0)
        busid="1-1.3.4.1"
    ;;

    dev1)
        busid="1-1.3.3.1"
    ;;

    dev2)
        busid="1-1.3.4.2.1"
    ;;

    dev3)
        busid="1-1.3.3.2.1"
    ;;

    dev4)
        busid="1-1.3.4.2.2"
    ;;

    dev5)
        busid="1-1.3.3.2.2"
    ;;

    dev6)
        busid="1-1.3.4.3"
    ;;

    dev7)
        busid="1-1.3.3.3"
    ;;

    dev8)
        busid="1-1.3.4.2.3"
    ;;

    dev9)
        busid="1-1.3.3.2.3"
    ;;

    dev10)
        busid="1-1.3.4.4"
    ;;

    dev11)
        busid="1-1.3.3.2.4"
    ;;

    *)
        echo "dev:$dev is not supported by $fru"
        exit -1
    ;;
    esac
}

function get_cwc_bus_id() {
    if [ $exp != "0x04" ]; then
        echo "command not supported by this platform"
        exit -1
    fi

    case $dev in
    pesw-uart)
        busid="1-1.3.2.2"
    ;;

    *)
        echo "dev:$dev is not supported by $fru"
        exit -1
    ;;
    esac
}

function get_top_gpv3_bus_id() {
    if [ $exp != "0x04" ]; then
        echo "command not supported by this platform"
        exit -1
    fi

    case $dev in
    pesw-uart)
        busid="1-1.3.1.4.2.4"
    ;;

    uart0)
        busid="1-1.3.1.1"
    ;;

    uart1)
        busid="1-1.3.1.2.2"
    ;;

    uart2)
        busid="1-1.3.1.2.4"
    ;;

    dev0)
        busid="1-1.3.1.4.1"
    ;;

    dev1)
        busid="1-1.3.1.3.1"
    ;;

    dev2)
        busid="1-1.3.1.4.2.1"
    ;;

    dev3)
        busid="1-1.3.1.3.2.1"
    ;;

    dev4)
        busid="1-1.3.1.4.2.2"
    ;;

    dev5)
        busid="1-1.3.1.3.2.2"
    ;;

    dev6)
        busid="1-1.3.1.4.3"
    ;;

    dev7)
        busid="1-1.3.1.3.3"
    ;;

    dev8)
        busid="1-1.3.1.4.2.3"
    ;;

    dev9)
        busid="1-1.3.1.3.2.3"
    ;;

    dev10)
        busid="1-1.3.1.4.4"
    ;;

    dev11)
        busid="1-1.3.1.3.2.4"
    ;;

    *)
        echo "dev:$dev is not supported by $fru"
        exit -1
    ;;
    esac
}

function get_bot_gpv3_bus_id() {
    if [ $exp != "0x04" ]; then
        echo "command not supported by this platform"
        exit -1
    fi

    case $dev in
    pesw-uart)
        busid="1-1.3.4.4.2.4"
    ;;

    uart0)
        busid="1-1.3.4.1"
    ;;

    uart1)
        busid="1-1.3.4.2.2"
    ;;

    uart2)
        busid="1-1.3.4.2.4"
    ;;

    dev0)
        busid="1-1.3.4.4.1"
    ;;

    dev1)
        busid="1-1.3.4.3.1"
    ;;

    dev2)
        busid="1-1.3.4.4.2.1"
    ;;

    dev3)
        busid="1-1.3.4.3.2.1"
    ;;

    dev4)
        busid="1-1.3.4.4.2.2"
    ;;

    dev5)
        busid="1-1.3.4.3.2.2"
    ;;

    dev6)
        busid="1-1.3.4.4.3"
    ;;

    dev7)
        busid="1-1.3.4.3.3"
    ;;

    dev8)
        busid="1-1.3.4.4.2.3"
    ;;

    dev9)
        busid="1-1.3.4.3.2.3"
    ;;

    dev10)
        busid="1-1.3.4.4.4"
    ;;

    dev11)
        busid="1-1.3.4.3.2.4"
    ;;

    *)
        echo "dev:$dev is not supported by $fru"
        exit -1
    ;;
    esac
}

function bind_usb() {
    if [ -z $fru ] || [ -z $dev ]; then
        show_usage
        exit -1
    fi

    exp=$(get_2ou_board_type 4)

    case $fru in
    slot1-2U)
        get_2u_gpv3_bus_id
    ;;

    slot1-2U-exp)
        get_cwc_bus_id
    ;;

    slot1-2U-top)
        get_top_gpv3_bus_id
    ;;

    slot1-2U-bot)
        get_bot_gpv3_bus_id
    ;;

    *)
        show_usage
        exit -1
    ;;
    esac

    if [ $cmd == "unbind" ] ; then
        usbip unbind --busid $busid
    else
        hub=$(/usr/bin/bic-util slot1 --get_gpio | grep 16 | awk '{print $3;}') # get gpio value

        if [ $hub -eq 0 ]; then
            /usr/bin/bic-util slot1 --set_gpio 16 1 > /dev/null 2>&1
            retry=15
            echo "Initializing USB devs..."
            sleep 5
        fi

        check_file_exist

        modprobe usbip-core
        modprobe usbip-host
        usbipd -D
        sleep 1
        usbip bind --busid $busid
    fi
}

function get_bus_id() {
    exp=$(get_2ou_board_type 4)

    case $fru in
    slot1-2U)
        get_2u_gpv3_bus_id
    ;;

    slot1-2U-exp)
        get_cwc_bus_id
    ;;

    slot1-2U-top)
        get_top_gpv3_bus_id
    ;;

    slot1-2U-bot)
        get_bot_gpv3_bus_id
    ;;

    *)
        show_usage
        exit -1
    ;;
    esac

    echo "$fru $dev usb bus id is $busid"
}

case $cmd in
    bind)
        bind_usb
    ;;

    unbind)
        bind_usb
    ;;

    bus-id)
        get_bus_id
    ;;

    *)
        show_usage
        exit -1
    ;;
esac
