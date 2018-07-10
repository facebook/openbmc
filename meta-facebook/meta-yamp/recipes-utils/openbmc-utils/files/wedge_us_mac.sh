#!/bin/bash

bmc_mac=`cat /sys/class/net/eth0/address`
# YAMP Uses NC-SI interface. So US MAC is always BMC - 1
offset=-1
for i in 6 5 4 3 2 1 
do
  # First, dissolve mac address into hexadecimals
  mac[$i]=`echo $bmc_mac| cut -d ':' -f $i`
  int_val=$(( 16#${mac[$i]} ))
  # Do the new address calculation based on offset value
  int_val=$(( $int_val + $offset ))
  # Take care of overflow or underflow
  offset=0
  if [ $int_val -eq -1 ]; then
     int_val=255
     offset=-1
  fi
  if [ $int_val -eq 256 ]; then
     int_val=0
     offset=1
  fi
  mac[$i]=`printf '%02x' $int_val`
done
echo ${mac[1]}:${mac[2]}:${mac[3]}:${mac[4]}:${mac[5]}:${mac[6]}

