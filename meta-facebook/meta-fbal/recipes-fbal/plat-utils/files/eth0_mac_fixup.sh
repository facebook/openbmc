#!/bin/bash
#This is workaround for IPV4 lost There is slim chance ipv4 lost when AC cycle. 
#DHCP Client can't get OFFER package from server.
#This script is for reinitial ipv4 to prevnet it lost. 

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

KERNEL_VERSION=`uname -r`
if [[ ${KERNEL_VERSION} != 4.1.* ]]; then
  echo "fixup eth0 interface.."
  for i in {1..10};
  do
    ip=$(ip -4 address |grep eth0 |grep inet)
    if [[ -z "$ip" ]]; then
      ifdown eth0
      ifup eth0
      sleep 3
    else
      break;
    fi
  done
fi
