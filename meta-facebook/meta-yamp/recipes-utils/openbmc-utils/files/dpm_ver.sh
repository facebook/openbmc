#!/bin/bash
. /usr/local/bin/openbmc-utils.sh

show_dps_ver() {
  name=$1
  bus=$2
  addr=$3
  drvload=$4
  if [ $drvload -eq 1 ]; then
     i2c_device_delete $bus $addr
     sleep 1
  fi
  dump=`i2cdump -y ${bus} ${addr} s 0x9e |grep -v abcdef`
  dps_ver=`echo ${dump##* }`
  echo $name $dps_ver
  if [ $drvload -eq 1 ]; then
     i2c_device_add $bus $addr ucd90120
  fi
}
echo "!!!"
echo "!!! WARNING: Displaying DSP version will disrupt sensor reading !!!"
echo "!!!"
show_dps_ver "SUP.:" 9 0x11 0
show_dps_ver "SCD.:" 3 0x4e 1
show_dps_ver "PIM1:" 14 0x4e 1
show_dps_ver "PIM2:" 15 0x4e 1
show_dps_ver "PIM3:" 16 0x4e 1
show_dps_ver "PIM4:" 17 0x4e 1
show_dps_ver "PIM5:" 18 0x4e 1
show_dps_ver "PIM6:" 19 0x4e 1
show_dps_ver "PIM7:" 20 0x4e 1
show_dps_ver "PIM8:" 21 0x4e 1
echo "!!! Done."
