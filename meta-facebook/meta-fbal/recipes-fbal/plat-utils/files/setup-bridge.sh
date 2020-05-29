#!/bin/sh
. /usr/local/fbpackages/utils/ast-functions

usb_dev_lost() {
  while [ 1 ]; do
    usb_pre=$(ifconfig -a |grep usb)

    if [ "$usb_pre" == "" ]; then
      echo ==Lost Usb Ethernet Device==
      break   
    fi
  done
  usb_dev_delect
}

usb_dev_delect() {
  while [ 1 ]; do
    usb_pre=$(ifconfig -a |grep usb)

    if [ "$usb_pre" != "" ]; then
      echo ==Find Usb Ethernet Device==
      $(brctl addif br0 usb0)
      $(ip link set up usb0)
      break   
    fi
  done
  usb_dev_lost
}

#Detect Cable Present
while [ 1 ]; do 
  cable_pre1=$(($(gpio_get PRSNT_PCIE_CABLE_1_N)))
  cable_pre2=$(($(gpio_get PRSNT_PCIE_CABLE_2_N)))

  #For Test
  #cable_pre1=0

  if [[ "$cable_pre1" -eq 0 ]] || [[ "$cable_pre2" -eq 0 ]]; then
    echo ==Usb Cable Present==
    break
  fi
done

#Detect USB Ethernet Device
while [ 1 ]; do
  usb_pre=$(ifconfig -a |grep usb)
  #For Test
  #usb_pre="usb"

  if [ "$usb_pre" != "" ]; then
    echo ==Find Usb Ethernet Device==
    break   
  fi
done

echo "Initial IP Bridge"

#Get FBEP MAC Address
while [ 1 ]; do
  mac_ep=$(/usr/bin/ipmitool raw 0x30 0x34 0x0A 0x0c 0x02 0x00 0x05 2>&1)
  #For Test
  #mac_ep="11 78 03 9B 96 FC 99"

  if [[ "Unable" != *$mac_ep* ]] || [[ "00 00 00 00 00 00 00" != *$mac_ep* ]]; then
    echo "==Get FBEP MAC address=="
    break
  fi
done

#Set FBEP MAC Filter
mac_ep=$(echo $mac_ep | awk '{for (i=2; i <= NF; i++) {printf "0x"$i" "}}') 
echo "EP MAC Address = "$mac_ep
echo "Set EP Mac Address Filter"
/usr/local/bin/ncsi-util 0x0e $mac_ep 0x02 0x01

#IP Bridge
$(ip link set down eth0)
$(ip link set down usb0)
$(brctl addbr br0)
$(brctl addif br0 eth0)
$(brctl addif br0 usb0)
ifconfig br0 hw ether $(cat /sys/class/net/eth0/address)
$(ip link set up eth0)
$(ip link set up usb0)
$(ip link set up br0)

#Clear ETH0 Client
sv stop dhc6
ip -6 addr flush dev eth0
ip -4 addr flush dev eth0
killall dhclient

#Restart DHCP
echo DHCP IPV6..
dhclient -6 -d -D LL --address-prefix-len 64 -pf /var/run/dhclient6.br0.pid br0 &
echo DHCP IPV4..
dhclient -d -pf /var/run/dhclient.br0.pid br0 &


echo Detect USB Lost
usb_dev_lost
