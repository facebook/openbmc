#!/bin/bash
#
# This script will wake up every 10 seconds and do the following
#    1. Try to discover any new PIM(s) plugged in
#    2. For all discovered PIMs, attempt to turn on the PIM and 
#       the gearbox chip on it
#

. /usr/local/bin/openbmc-utils.sh
SCD_DRV_PATH=/sys/bus/i2c/drivers/scdcpld/4-0023
pim_list=(0 1 2 3 4 5 6 7)
pim_found=(0 0 0 0 0 0 0 0)
pim_addr=(14-0020 15-0020 16-0020 17-0020 18-0020 19-0020 20-0020 21-0020)
pim_bus=(14 15 16 17 18 19 20 21)
pca_addr=20
pim_on_gpio=(0 0 0 0 0 0 0 0)
pim_gb_on_gpio=(0 0 0 0 0 0 0 0)
while true; do
  # 1. For any uncovered PIM, try to discover
  for i in ${pim_list[@]}
  do
    if [ "${pim_found[$i]}" -eq "0" ]; then
      pim_number=$(expr $i + 1)
      pim_addr=${pim_bus[$i]}-00$pca_addr  # 14-0020 for example
      # Check if Switch card senses the PIM presence
      pim_path="$SCD_DRV_PATH/lc$pim_number""_prsnt_sta"
      pim_present=`cat $pim_path | head -n 1`
      if [ "$pim_present" == "0x1" ]; then
         dev_path=/sys/bus/i2c/devices/$pim_addr
         drv_path=/sys/bus/i2c/drivers/pca953x/$pim_addr
         if [ ! -e $drv_path ]; then
           # Driver doesn't exist
           if [ -e $dev_path ]; then
              # Device exists. Meaning previous probe failed. Clean up previous failure first
              i2c_device_delete ${pim_bus[$i]} 0x$pca_addr
              # Give some time to kernel
              sleep 1
           fi
           # Probe this device
           reg0_val=`i2cget -y ${pim_bus[$i]} 0x$pca_addr 0 2>&1`
           if [[ ! "$reg0_val" == *"Error"* ]]; then
              i2c_device_add ${pim_bus[$i]} 0x$pca_addr pca9555
           fi
         fi
         # Check if device was probed and driver was installed
         if [ -e $drv_path/gpio ]; then
            base=`cat $drv_path/gpio/gpiochip*/base 2>/dev/null`
            # Base should be at least bigger than 100, do quick check
            if [ $base -gt 100 ]; then 
              pim_on_gpio[$i]=$(expr $base + 1)
              pim_gb_on_gpio[$i]=$(expr $base + 8)
              echo ${pim_on_gpio[$i]} > /sys/class/gpio/export
              echo ${pim_gb_on_gpio[$i]} > /sys/class/gpio/export
              pim_found[$i]=1
              echo pim_enable: registered PIM$pim_number ${pim_on_gpio[$i]} , ${pim_gb_on_gpio[$i]}
            fi
         fi
      fi
    fi
  done
  # 2. For discovered gpios, try turning it on
  for i in ${pim_list[@]}
  do
    if [ "${pim_found[$i]}" -eq "1" ]; then
      gpio_set ${pim_on_gpio[$i]} 1
      sleep 1
      gpio_set ${pim_gb_on_gpio[$i]} 1
    fi
  done
  # Sleep 10 seconds until the next loop
  sleep 10
done
