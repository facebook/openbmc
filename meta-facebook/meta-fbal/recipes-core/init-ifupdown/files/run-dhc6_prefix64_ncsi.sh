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
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#
logger "run-dhc6.sh: dhclient -6 Started.."
pid="/var/run/dhclient6.eth0.pid"

# shellcheck disable=SC2086
mac_to_ll() {
  IFS=':'; set $1; unset IFS
  echo "fe80::$(printf %x $((0x$1 ^ 2)))$2:${3}ff:fe$4:$5$6" | \
    sed -E 's/:0+/:/g; s/:{3,}/::/; s/:$/:0/'
}

exp_ll=
ll=

for _ in {1..30}; do
  # wait getting address from NC-SI
  if [ "$(cat /sys/class/net/eth0/addr_assign_type)" = "3" ]; then
    if [ "$(cat /sys/class/net/eth0/address)" != "$(cat /sys/class/net/br0/address)" ]; then
      ifconfig br0 hw ether "$(cat /sys/class/net/eth0/address)"
    fi
    exp_ll=$(mac_to_ll "$(cat /sys/class/net/eth0/address)")
    ll=$(ip -6 addr show dev eth0 scope link | sed -e's/^.*inet6 \([^ ]*\)\/.*$/\1/;t;d')
    br_ll=$(ip -6 addr show dev br0 scope link | sed -e's/^.*inet6 \([^ ]*\)\/.*$/\1/;t;d')
    # re-generate link-local address
    if [ "$ll" != "$exp_ll" ]; then
      ip -6 addr flush dev eth0
      echo 1 > /proc/sys/net/ipv6/conf/eth0/addr_gen_mode
      echo 0 > /proc/sys/net/ipv6/conf/eth0/addr_gen_mode
    fi
    if [ "$br_ll" != "$exp_ll" ]; then
      ip -6 addr flush dev br0
      echo 1 > /proc/sys/net/ipv6/conf/br0/addr_gen_mode
      echo 0 > /proc/sys/net/ipv6/conf/br0/addr_gen_mode
    fi
    break;
  fi
  sleep 1
done

if [[ "$ll" != "$exp_ll" || "$br_ll" != "$exp_ll" ]]; then
  for _ in {1..100}; do
    usleep 10000
    ll=$(ip -6 addr show dev eth0 scope link | sed -e's/^.*inet6 \([^ ]*\)\/.*$/\1/;t;d')
    br_ll=$(ip -6 addr show dev br0 scope link | sed -e's/^.*inet6 \([^ ]*\)\/.*$/\1/;t;d')
    if [[ "$ll" = "$exp_ll" && "$br_ll" = "$exp_ll" ]]; then
      break;
    fi
  done
fi

exec dhclient -6 -d -D LL --address-prefix-len 64 -pf ${pid} br0 "$@"
