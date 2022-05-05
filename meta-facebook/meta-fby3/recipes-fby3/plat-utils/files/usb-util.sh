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
BOARD_ID=$(get_bmc_board_id)

function show_usage() {
  if [ "$BOARD_ID" -eq 9 ]; then
    #The BMC of class2
    echo "Usage: usb-util.sh [ bind | unbind ] [ slot1-2U | slot1-2U-exp | slot1-2U-top | slot1-2U-bot ] [ pesw-uart, uart0, uart1, uart2, dev0, dev1, ..., dev11 ]"
    echo "Usage: usb-util.sh bus-id [ slot1-2U | slot1-2U-exp | slot1-2U-top | slot1-2U-bot ] [ pesw-uart, uart0, uart1, uart2, dev0, dev1, ..., dev11 ]"
  else
    #The BMC of class1
    echo "Usage: usb-util.sh [ bind | unbind ] [ slot1-2U | slot3-2U ] [ dev0, dev1, ..., dev11 ]"
    echo "Usage: usb-util.sh bus-id [ slot1-2U | slot3-2U  ] [ dev0, dev1, ..., dev11 ]"
  fi
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

function get_2u_gpv3_bus_id_by_fru() {
    if [ $exp != "0x00" ] && [ $exp != "0x03" ]; then
        echo "command not supported by this platform"
        exit -1
    fi

    if [ "$fru" = "slot1-2U" ]; then
      case $dev in
        dev0) # a
        busid="1-1.1.3.4.1"
      ;;
        dev1) # b
        busid="1-1.1.3.3.1"
      ;;
        dev2) # c
        busid="1-1.1.3.4.2.1"
      ;;
        dev3) # d
        busid="1-1.1.3.3.2.1"
      ;;
        dev4) # e
        busid="1-1.1.3.4.2.2"
      ;;
        dev5) # f
        busid="1-1.1.3.3.2.2"
      ;;
        dev6) # g
        busid="1-1.1.3.4.3"
      ;;
        dev7) # h
        busid="1-1.1.3.3.3"
      ;;
        dev8) # i
        busid="1-1.1.3.4.2.3"
      ;;
        dev9) # j
        busid="1-1.1.3.3.2.3"
      ;;
        dev10) # k
        busid="1-1.1.3.4.4"
      ;;
        dev11) # l
        busid="1-1.1.3.3.2.4"
      ;;
        *)
        echo "dev:$dev is not supported by $fru"
        exit -1
      ;;
      esac
    elif [ "$fru" = "slot3-2U" ]; then
      case $dev in
        dev0) # a
        busid="1-1.3.3.4.1"
      ;;
        dev1) # b
        busid="1-1.3.3.3.1"
      ;;
        dev2) # c
        busid="1-1.3.3.4.2.1"
      ;;
        dev3) # d
        busid="1-1.3.3.3.2.1"
      ;;
        dev4) # e
        busid="1-1.3.3.4.2.2"
      ;;
        dev5) # f
        busid="1-1.3.3.3.2.2"
      ;;
        dev6) # g
        busid="1-1.3.3.4.3"
      ;;
        dev7) # h
        busid="1-1.3.3.3.3"
      ;;
        dev8) # i
        busid="1-1.3.3.4.2.3"
      ;;
        dev9) # j
        busid="1-1.3.3.3.2.3"
      ;;
        dev10) # k
        busid="1-1.3.3.4.4"
      ;;
        dev11) # l
        busid="1-1.3.3.3.2.4"
      ;;
        *)
        echo "dev:$dev is not supported by $fru"
        exit -1
      ;;
      esac
    else
      echo "The $fru is invalid"
      exit -1;
    fi

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

    if [ "$fru" = "slot3-2U" ]; then
        exp=$(get_2ou_board_type 6)
    else
        exp=$(get_2ou_board_type 4)
    fi

    case $fru in
    slot1-2U)
        if [ "$BOARD_ID" -eq 9 ]; then
            get_2u_gpv3_bus_id
        else
            get_2u_gpv3_bus_id_by_fru
        fi
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

    slot3-2U)
        get_2u_gpv3_bus_id_by_fru
    ;;

    *)
        show_usage
        exit -1
    ;;
    esac

    if [ $cmd == "unbind" ] ; then
        usbip unbind --busid $busid
    else
        if [ "$fru" = "slot3-2U" ]; then
            hub=$(/usr/bin/bic-util slot3 --get_gpio | grep 16 | awk '{print $3;}') # get gpio value
        else
            hub=$(/usr/bin/bic-util slot1 --get_gpio | grep 16 | awk '{print $3;}') # get gpio value
        fi

        if [ $hub -eq 0 ]; then
            if [ "$fru" = "slot3-2U" ]; then
                /usr/bin/bic-util slot3 --set_gpio 16 1 > /dev/null 2>&1
            else
                /usr/bin/bic-util slot1 --set_gpio 16 1 > /dev/null 2>&1
            fi
            retry=15
            echo "Initializing USB devs..."
            sleep 10
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
    if [ "$fru" = "slot3-2U" ]; then
        exp=$(get_2ou_board_type 6)
    else
        exp=$(get_2ou_board_type 4)
    fi

    case $fru in
    slot1-2U)
        if [ "$BOARD_ID" -eq 9 ]; then
            get_2u_gpv3_bus_id
        else
            get_2u_gpv3_bus_id_by_fru
        fi
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

    slot3-2U)
        get_2u_gpv3_bus_id_by_fru
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
