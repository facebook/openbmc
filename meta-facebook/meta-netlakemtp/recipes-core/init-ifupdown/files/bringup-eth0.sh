#!/bin/bash

logger "Update eth0 IPv6 link local address after NC-SI MAC address configured"

# shellcheck disable=SC2086
mac_to_ll() {
  IFS=':'; set $1; unset IFS
  echo "fe80::$(printf %x $((0x$1 ^ 2)))$2:${3}ff:fe$4:$5$6" | \
    sed -E 's/:0+/:/g; s/:{3,}/::/; s/:$/:0/'
}

exp_ll=""
ll=""

for _ in {1..30}; do
  # wait getting address from NC-SI
  if [ "$(cat /sys/class/net/eth0/addr_assign_type)" = "3" ]; then
    exp_ll=$(mac_to_ll "$(cat /sys/class/net/eth0/address)")
    rc_exp_ll=$?
    ll=$(ip -6 addr show dev eth0 scope link | sed -e's/^.*inet6 \([^ ]*\)\/.*$/\1/;t;d')
    rc_ll=$?

    if [ "$ll" = "$exp_ll" ]; then
      break;
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
  for i in {1..100}; do
    usleep 10000
    ll=$(ip -6 addr show dev eth0 scope link | sed -e's/^.*inet6 \([^ ]*\)\/.*$/\1/;t;d')
    if [ "$ll" = "$exp_ll" ]; then
      break;
    elif [ "$ll" != "$exp_ll" ] && [ "$i" -eq 100 ]; then
      logger "Failed to set address from MAC"
    else
      continue;
    fi
  done
fi

logger "Restart eth0 after MAC and Linklock addr update"
ifdown eth0
sleep 1
ifup eth0 &
