#!/bin/bash
#This is workaround for IPV4 lost There is slim chance ipv4 lost when AC cycle.
#DHCP Client can't get OFFER package from server.
#This script is for reinitial ipv4 to prevnet it lost.

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

check_eth0_ipv4() {
  for i in {1..10};
  do
    ip=$(ifconfig eth0 |grep "inet addr")
    if [[ -z "$ip" ]]; then
      pid=$(cat /var/run/dhclient.eth0.pid)
      kill $pid
      dhclient -d -pf /var/run/dhclient.eth0.pid eth0 > /dev/null 2>&1 &
      sleep 3
    else
      break;
    fi
  done
}

echo "check eth0 ipv4..."
check_eth0_ipv4 &
