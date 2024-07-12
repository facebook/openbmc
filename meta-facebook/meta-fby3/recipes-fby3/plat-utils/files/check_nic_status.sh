#!/bin/bash
#This is workaround for NIC power lost when AC cycle.
#This script is for reinitial NIC power, rebind NIC temp driver and reconfig network

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

# shellcheck source=common/recipes-utils/openbmc-utils/files/i2c-utils.sh
. /usr/local/bin/i2c-utils.sh
. /usr/local/fbpackages/utils/ast-functions

#Check NIC status by verifying the existence of the NIC version
check_nic_status() {
  local cpld_bus=$1

  echo "check NIC status..."

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
      i2cset -f -y $cpld_bus 0x0f 0x10 0x1
      sleep 1
      i2cset -f -y $cpld_bus 0x0f 0x10 0x0

      sleep 5 #Wait for NIC power on
      if [ ! -L "${SYSFS_I2C_DEVICES}/8-001f/driver" ]; then
        if i2c_bind_driver tmp421 8-001f 4 2>/dev/null; then
          echo "rebind 8-001f to driver tmp421 successfully"
        fi
      fi
      ifdown eth0
      ifup eth0
      logger -t "nic_check" -p daemon.crit "Reinit NIC..."
      sleep 5 #Wait for the NCSI initialization
    fi
  done
}

bmc_location=$(get_bmc_board_id)
if [ $bmc_location -eq 9 ]; then
  # class 2
  check_nic_status 9
else
  check_nic_status 12
fi
