#!/bin/bash
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
#

input=$1


print_usage() {

    echo "  slot to host name lookup"
    echo ""
    echo "  Usage:      slot-util [ slot1, slot2, slot3, slot4, all ] : shows host for a particular/all slot(s)"
    echo "              slot-util [ 1, 2, 3, 4 ]                      : shows host for a particular slot"
    echo "              slot-util [ hostname ]                        : shows slot number for a host"
    echo "              slot-util --show-mac                          : shows MAC address assigned for each host"
}

if [ $# -eq 0 ]; then
    print_usage
    exit 1
fi

ocp3nic=0
sku_type=0

if grep -q "yosemite" /etc/issue; then
    sku_type=0
elif grep -q "fby35" /etc/issue; then
    sku_type=fffe
    ocp3nic=1
elif grep -q "fby3" /etc/issue; then
    sku_type=ffff
    ocp3nic=1
else
    # fby2 file
    # shellcheck disable=SC1091
    . /usr/local/fbpackages/utils/ast-functions

    # if no get_slot_type exist (such as Yosemite 1), assume 4 server config
    if [ -n "$(LC_ALL=C type -t get_slot_type)" ] && [ "$(LC_ALL=C type -t get_slot_type)" = function ]; then
      for i in $(seq 1 1 4)
      do
        tmp_sku="$(get_slot_type "$i")"
        sku_type="$(($(("$tmp_sku" << $(("$(("$i"*4))" - 4))))+"$sku_type"))"
      done
    fi
    # OCP3 NIC detection
    GPIOCLI_CMD=/usr/local/bin/gpiocli
    prsnt_a=$($GPIOCLI_CMD get-value --shadow MEZZ_PRSNTA2_N | awk -F= '{print $2}')
    prsnt_b=$($GPIOCLI_CMD get-value --shadow MEZZ_PRSNTB2_N | awk -F= '{print $2}')
    if ((prsnt_a == 0 && prsnt_b == 1)); then
      ocp3nic=1
    else
      ocp3nic=0
    fi
fi



# get BMC mac address and conver it to base 10 integer
bmc_mac_dec="$( printf "%d\\n" "0x$(sed s/://g "/sys/class/net/eth0/address")")"


# get neighboring devices in {hostname, MAC} format, shows only entries that can resolve to host name
host_table=$(ip -r neigh show dev eth0 | grep -iE 'com|edu|gov|org' | sort -k 3)


# calculate expected MAC address for each slot, convert it back to hex str
#
# MAC address calculation is different for 2xTL+2x carrier
case "$sku_type" in
   "0")
      if [ $ocp3nic -eq 1 ]; then
        echo "4 server, OCP3 NIC"
        slot1_mac=$(printf "%012x\\n" $((bmc_mac_dec-4)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//')
        slot2_mac=$(printf "%012x\\n" $((bmc_mac_dec-3)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//')
        slot3_mac=$(printf "%012x\\n" $((bmc_mac_dec-2)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//')
        slot4_mac=$(printf "%012x\\n" $((bmc_mac_dec-1)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//');
      else
        echo "4 server, OCP2 NIC"
        slot1_mac=$(printf "%012x\\n" $((bmc_mac_dec-1)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//')
        slot2_mac=$(printf "%012x\\n" $((bmc_mac_dec+1)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//')
        slot3_mac=$(printf "%012x\\n" $((bmc_mac_dec+3)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//')
        slot4_mac=$(printf "%012x\\n" $((bmc_mac_dec+5)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//')
      fi
   ;;
   "fffe")
      echo "Yosemite V3.5, OCP3 NIC"
      slot1_mac=$(printf "%012x\\n" $((bmc_mac_dec-1)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//')
      slot2_mac=$(printf "%012x\\n" $((bmc_mac_dec+1)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//')
      slot3_mac=$(printf "%012x\\n" $((bmc_mac_dec+3)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//')
      slot4_mac=$(printf "%012x\\n" $((bmc_mac_dec+5)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//')
   ;;
   "ffff")
      echo "Yosemite V3, OCP3 NIC"
      slot1_mac=$(printf "%012x\\n" $((bmc_mac_dec-1)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//')
      slot2_mac=$(printf "%012x\\n" $((bmc_mac_dec+1)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//')
      slot3_mac=$(printf "%012x\\n" $((bmc_mac_dec+3)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//')
      slot4_mac=$(printf "%012x\\n" $((bmc_mac_dec+5)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//')
   ;;
   "1028")
      echo "2server + 2GPv2, OCP2 NIC"
      slot2_mac=$(printf "%012x\\n" $((bmc_mac_dec-1)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//')
      slot4_mac=$(printf "%012x\\n" $((bmc_mac_dec+1)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//')
   ;;
   *)
      echo "unknown config - use default"
      slot1_mac=$(printf "%012x\\n" $((bmc_mac_dec-1)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//')
      slot2_mac=$(printf "%012x\\n" $((bmc_mac_dec+1)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//')
      slot3_mac=$(printf "%012x\\n" $((bmc_mac_dec+3)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//')
      slot4_mac=$(printf "%012x\\n" $((bmc_mac_dec+5)) | sed -e 's/[0-9A-Fa-f]\{2\}/&:/g' -e 's/:$//')
   ;;
esac

if [ "$1" == "--show-mac" ]; then
  for i in 1 2 3 4
  do
    slotmac="slot$i"
    slotmac="${slotmac}_mac"
    eval echo slot$i MAC: "${!slotmac}"
  done
  exit 1
fi

# match host name to each slot based on MAC address
if [[ -n $slot1_mac ]]; then
  # shellcheck disable=SC2034
  host1="$(echo "$host_table" | grep -i "${slot1_mac}" | cut  -d " " -f1)"
fi
if [[ -n $slot2_mac ]]; then
  # shellcheck disable=SC2034
  host2="$(echo "$host_table" | grep -i "${slot2_mac}" | cut  -d " " -f1)"
fi
if [[ -n $slot3_mac ]]; then
  # shellcheck disable=SC2034
  host3="$(echo "$host_table" | grep -i "${slot3_mac}" | cut  -d " " -f1)"
fi
if [[ -n $slot4_mac ]]; then
  # shellcheck disable=SC2034
  host4="$(echo "$host_table" | grep -i "${slot4_mac}" | cut  -d " " -f1)"
fi

# check if user asked for a specific slot, either in "slotX", or just "X"
if [[ "$input" =~ ^[0-9]+$ ]]; then
    slot_num=$input
elif [[ $1 == "slot"* ]]; then
    slot_num=$(echo "$1" | tr -dc '0-9')
fi


# check if user specified "all" option
if [ -z "$slot_num" ]; then
    if [[ $1 == "all" ]]; then
        for i in 1 2 3 4
        do
           eval echo slot$i: \$host$i
        done
    else
        # check if user entered a valid host name
        match=$(echo "$host_table" | grep -i "$1")
        # if no match
        if [ -z "$match" ]; then
            echo host name "$1" not found
        else
            for i in 1 2 3 4
            do
               hostname=$( (eval echo \$host$i) | grep -i "$1")
               if [ -z "$hostname" ]; then
                   :
               else
                   eval echo slot$i: \$host$i
               fi
            done
        fi
    fi
else
    # user specified a specific slot
    eval echo slot"$slot_num": \$host"$slot_num"
fi
