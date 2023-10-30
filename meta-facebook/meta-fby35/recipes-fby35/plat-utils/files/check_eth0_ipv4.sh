#!/bin/bash
#This is workaround for IPV4 lost There is slim chance ipv4 lost when AC cycle.
#DHCP Client can't get OFFER package from server.
#This script is for reinitial ipv4 to prevnet it lost.

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

#This is a workaround when NIC lost power for now
#Check NIC status by verifying the existence of the NIC version
check_nic_status() {
  for i in {1..4};
  do
    if [ "$i" == 4 ]; then
      logger -t "nic_check" -p daemon.crit "Fail to power on NIC"
      break;
    fi
    /usr/bin/fw-util nic --version
    if [ -f "/tmp/cache_store/nic_fw_ver" ]; then
      break;
    else
      i2cset -f -y 12 0x0f 0x10 0x1
      sleep 1
      i2cset -f -y 12 0x0f 0x10 0x0
      echo 1e690000.ftgmac > /sys/bus/platform/drivers/ftgmac100/bind
      sleep 5 #Wait for NIC power on
      /etc/init.d/networking restart
      logger -t "nic_check" -p daemon.crit "Reinit NIC..."
      sleep 5 #Wait for the NCSI initialization
    fi
  done
}

check_eth0_ipv4() {
  for i in {1..10};
  do
    ip=$(ifconfig eth0 |grep "inet addr")
    if [[ -z "$ip" ]]; then
      pid=$(cat /var/run/dhclient.eth0.pid)
      kill "$pid"
      dhclient -d -pf /var/run/dhclient.eth0.pid eth0 > /dev/null 2>&1 &
      sleep 3
    else
      break;
    fi
  done
}

echo "check NIC status..."
check_nic_status

echo "check eth0 ipv4..."
check_eth0_ipv4 &
