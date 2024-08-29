#!/bin/bash
# This is workaround for NIC power lost when AC cycle.
# This script is use to reinitial NIC power and reconfig network.

. /usr/local/fbpackages/utils/ast-functions

# Check NIC status by verifying the existence of the NIC version
check_nic_status() {

  echo "Checking NIC power status..."

  if [ "$(gpio_get NIC_PRSNTB3_N)" == "1" ]; then
    logger -t "check_nic_status" -p daemon.crit "NIC card is missing"
    exit 0
  fi

  for retry in {1..5};
  do
    if [ "$retry" == "5" ]; then
      logger -t "check_nic_status" -p daemon.crit "Fail to power on NIC"
      break;
    fi

    # Check NIC firmware version
    /usr/bin/fw-util nic --version
    if [ -f "/tmp/cache_store/nic_fw_ver" ]; then
      break;
    else 
      # Power cycle NIC
      gpio_set BMC_NIC_FULL_PWR_EN_R 0
      sleep 1
      gpio_set BMC_NIC_FULL_PWR_EN_R 1

      # Wait for NIC power on
      sleep 5

      ifdown eth0
      ifup eth0

      logger -t "check_nic_status" -p daemon.crit "Reinit NIC..."

      #Wait for the NCSI initialization
      sleep 5
    fi
  done
}

check_nic_status
