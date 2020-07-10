#!/bin/sh
. /usr/local/fbpackages/utils/ast-functions

usb_dev_lost() {
  while [ 1 ]; do
    usb_pre=$(ifconfig -a | grep usb)

    if [ "$usb_pre" == "" ]; then
      echo "==Lost Usb Ethernet Device=="
      break
    fi

    sleep 10
  done
}

usb_dev_detect() {
  while [ 1 ]; do
    usb_pre=$(ifconfig -a | grep usb)

    if [ "$usb_pre" != "" ]; then
      echo "==Find Usb Ethernet Device=="
      break
    fi

    sleep 10
  done
}


#Detect Cable Present
while [ 1 ]; do
  cable_pre1=$(($(gpio_get PRSNT_PCIE_CABLE_1_N)))
  cable_pre2=$(($(gpio_get PRSNT_PCIE_CABLE_2_N)))

  if [[ "$cable_pre1" -eq 0 || "$cable_pre2" -eq 0 ]]; then
    echo "==Usb Cable Present=="
    break
  fi

  sleep 10
done

#Detect USB Ethernet Device
usb_dev_detect


echo "Initialize IP Bridge..."

#Get EP MAC Address
while [ 1 ]; do
  mac=($(/usr/bin/ipmitool raw 0x30 0x34 0x0a 0x0c 0x02 0x00 0x05 2>/dev/null))
  #mac=(11 78 03 9B 96 FC 99)
  if [[ ${#mac[@]} -ge 7 && ${mac[0]} == "11" && $((16#${mac[1]} & 1)) -ne 1 ]]; then
    echo "==Get EP MAC address=="
    break
  fi

  sleep 3
done

#Set MAC Filter for EP
mac_ep=(${mac[@]/#/0x})
echo "Set MAC Address Filter for EP: ${mac_ep[@]:1}"
/usr/local/bin/ncsi-util 0x0e ${mac_ep[@]:1} 0x02 0x01

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

#Clear eth0 Client
sv stop dhc6
ip -6 addr flush dev eth0
ip -4 addr flush dev eth0
killall dhclient

#Restart DHCP
echo "DHCPv6..."
sv start dhc6
echo "DHCPv4..."
dhclient -d -pf /var/run/dhclient.br0.pid br0 &


echo "Detect USB Lost..."
while [ 1 ]; do
  usb_dev_lost
  usb_dev_detect
  $(brctl addif br0 usb0)
  $(ip link set up usb0)
  sleep 5
done
