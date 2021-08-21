#!/bin/bash
#This is workaround for IPV4 lost There is slim chance ipv4 lost when AC cycle. 
#DHCP Client can't get OFFER package from server.
#This script is for reinitial ipv4 to prevnet it lost. 

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

KERNEL_VERSION=$(uname -r)
if [[ ${KERNEL_VERSION} != 4.1.* ]]; then
  for _ in {1..10}; do
    ip=$(ip -4 address |grep br0 |grep inet)
    if [ -z "$ip" ]; then
      # shellcheck disable=SC2009
      vid=$(ps |grep "dhclient.eth0" |grep -v grep |awk '{print $1}')
      kill "$vid"
      dhclient -d -pf /var/run/dhclient.eth0.pid br0 > /dev/null 2>&1 &
      sleep 10
    else
      break;
    fi
  done
fi
