#!/bin/bash

# shellcheck disable=SC1091
. /usr/local/fbpackages/utils/ast-functions


asic_mac_check() {
  mac3=$(ncsi-util 0x17 | awk -F'mac_3 = ' '{print $2}')

  if [[ "$mac3" -eq "0x0" ]]
  then
    echo "ASIC MAC address recovery!"
    /usr/local/bin/ncsi-util 0x0e "${mac_peer[@]:1}" 0x02 0x01 > /dev/null 2>&1 &
  fi

}

usb_dev_lost() {
  while true; do
    usb_pre=$(ifconfig -a | grep usb)

    if [ "$usb_pre" == "" ]; then
      echo "==Lost Usb Ethernet Device=="
      break
    else
      asic_mac_check
    fi

    sleep 10
  done
}

usb_dev_detect() {
  while true; do
    usb_pre=$(ifconfig -a | grep usb)

    if [ "$usb_pre" != "" ]; then
      echo "==Find Usb Ethernet Device=="
      break
    fi

    sleep 10
  done
}


#Detect Cable Present
while true; do
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

#Get EP/CC MAC Address
position=$(($(/usr/bin/kv get mb_pos)))
mode=$(($(/usr/bin/kv get mb_skt) >> 1))  #2S:mode=2, 4S_EX:mode=1, 4S_EP:mode=0

if [[ "$position" -eq 0 || "$mode" -eq 2 ]]; then
  peer_name="EP"
  peer_addr="0x2c"
else
  peer_name="CC"
  peer_addr="0x2e"
fi

while true; do
  echo "Get $peer_name MAC"
  IFS=" " read -r -a mac <<< "$(/usr/local/bin/ipmb-util 6 $peer_addr 0x30 0x02 0x00 0x05 2>/dev/null)"
  #mac=(11 78 03 9B 96 FC 99)
  if [[ ${#mac[@]} -ge 7 && ${mac[0]} == "11" && $((16#${mac[1]} & 1)) -ne 1 ]]; then
    if [ "${mac[*]:1}" != "00 00 00 00 00 00" ]; then
      echo "==Get MAC address Success=="
      break
    fi
  fi

  sleep 3
done

#Set MAC Filter for EP/CC
mac_peer=("${mac[@]/#/0x}")
echo "Set MAC Address Filter for $peer_name: ${mac_peer[*]:1}"
/usr/local/bin/ncsi-util 0x0e "${mac_peer[@]:1}" 0x02 0x01

brctl addif br0 usb0
ip link set up usb0


echo "Detect USB Lost..."
while true; do
  usb_dev_lost
  usb_dev_detect
  brctl addif br0 usb0
  ip link set up usb0
  sleep 5
done
