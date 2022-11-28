#!/bin/bash

# NC-SI version (0x15) field 52 contains the IANA of the vendor.
# If this is 00000000, then we are running in QEMU and there is no point
# to wait.
if [[ "$(ncsi-util 0x15 | grep 52:)" == *"0x00 0x00 0x00 0x00"* ]]; then
    exit 0
fi

logger "Update eth0 IPv6 link local address after NC-SI MAC address configured"

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
    exp_ll=$(mac_to_ll "$(cat /sys/class/net/eth0/address)")
    ll=$(ip -6 addr show dev eth0 scope link | sed -e's/^.*inet6 \([^ ]*\)\/.*$/\1/;t;d')
    if [ "$ll" = "$exp_ll" ]; then
      break
    fi
    # re-generate link-local address
    ip -6 addr flush dev eth0 scope link
    echo 1 > /proc/sys/net/ipv6/conf/eth0/addr_gen_mode
    echo 0 > /proc/sys/net/ipv6/conf/eth0/addr_gen_mode
    break;
  fi
  sleep 1
done

if [ "$ll" != "$exp_ll" ]; then
  for _ in {1..100}; do
    usleep 10000
    ll=$(ip -6 addr show dev eth0 scope link | sed -e's/^.*inet6 \([^ ]*\)\/.*$/\1/;t;d')
    if [ "$ll" = "$exp_ll" ]; then
      break;
    fi
  done
fi

logger "restart eth0 after MAC and Linklock addr update"
ifdown eth0
sleep 1
ifup eth0 &
