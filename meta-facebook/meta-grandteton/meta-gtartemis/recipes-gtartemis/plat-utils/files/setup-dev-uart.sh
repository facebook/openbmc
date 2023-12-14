#!/bin/sh
#
# Copyright 2023-present Facebook. All Rights Reserved.
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

readonly EXIT_ERROR=255

DEV_MAP=(
    "cb_accl1_1:platform-1e6a3000.usb-usb-0:1.1.2.4.3.2:1."
    "cb_accl1_2:platform-1e6a3000.usb-usb-0:1.1.2.4.3.1:1."
    "cb_accl2_1:platform-1e6a3000.usb-usb-0:1.1.2.4.2.2:1."
    "cb_accl2_2:platform-1e6a3000.usb-usb-0:1.1.2.4.2.1:1."
    "cb_accl3_1:platform-1e6a3000.usb-usb-0:1.1.2.4.1.2:1."
    "cb_accl3_2:platform-1e6a3000.usb-usb-0:1.1.2.4.1.1:1."
    "cb_accl4_1:platform-1e6a3000.usb-usb-0:1.1.2.3.2:1."
    "cb_accl4_2:platform-1e6a3000.usb-usb-0:1.1.2.3.1:1."
    "cb_accl5_1:platform-1e6a3000.usb-usb-0:1.1.2.2.2:1."
    "cb_accl5_2:platform-1e6a3000.usb-usb-0:1.1.2.2.1:1."
    "cb_accl6_1:platform-1e6a3000.usb-usb-0:1.1.2.1.2:1."
    "cb_accl6_2:platform-1e6a3000.usb-usb-0:1.1.2.1.1:1."
    "cb_accl7_1:platform-1e6a3000.usb-usb-0:1.1.1.4.3.2:1."
    "cb_accl7_2:platform-1e6a3000.usb-usb-0:1.1.1.4.3.1:1."
    "cb_accl8_1:platform-1e6a3000.usb-usb-0:1.1.1.4.2.2:1."
    "cb_accl8_2:platform-1e6a3000.usb-usb-0:1.1.1.4.2.1:1."
    "cb_accl9_1:platform-1e6a3000.usb-usb-0:1.1.1.4.1.2:1."
    "cb_accl9_2:platform-1e6a3000.usb-usb-0:1.1.1.4.1.1:1."
    "cb_accl10_1:platform-1e6a3000.usb-usb-0:1.1.1.3.2:1."
    "cb_accl10_2:platform-1e6a3000.usb-usb-0:1.1.1.3.1:1."
    "cb_accl11_1:platform-1e6a3000.usb-usb-0:1.1.1.2.2:1."
    "cb_accl11_2:platform-1e6a3000.usb-usb-0:1.1.1.2.1:1."
    "cb_accl12_1:platform-1e6a3000.usb-usb-0:1.1.1.1.2:1."
    "cb_accl12_2:platform-1e6a3000.usb-usb-0:1.1.1.1.1:1."
)

UART_MAP=(
    "uart0:0-port0"
    "uart1:1-port0"
    "uart2:2-port0"
    "uart3:3-port0"
)

get_kv() {
  to_find=$1
  shift
  map=("$@")
  for kv in "${map[@]}" ; do
        key=${kv%%:*}
        val=${kv#*:}
        if [ "${key}" == "${to_find}" ]; then
            echo $val
            break
        fi
    done
}

get_dev() {
  get_kv "$1" "${DEV_MAP[@]}"
}

get_uart() {
  get_kv "$1" "${UART_MAP[@]}"
}

get_usb_dev() {
  dev=$1
  uart=$2
  dev_part=$(get_dev $dev)
  uart_part=$(get_uart $uart)
  if [ "$dev_part" == "" ]; then
    echo ""
  else
    if [ "${uart_part}" == "" ]; then
      echo ""
    else
      echo "${dev_part}${uart_part}"
    fi
  fi
}

start_dev_connect() {
  read -r PATH <<< "$(get_usb_dev "$1" "$2")"
  USB=$(/usr/bin/readlink /dev/serial/by-path/$PATH)
  USB=${USB:6}
  if [[ -z "$USB" ]]; then
    /bin/sleep 30 # wait for usb init
    exit "$EXIT_ERROR";
  else
    exec /usr/local/bin/mTerm_server ""$1"_"$2"" "/dev/$USB"
  fi
}

if [ "$3" == "start" ]; then
  start_dev_connect "$1" "$2"
else
  exit "$EXIT_ERROR";
fi
