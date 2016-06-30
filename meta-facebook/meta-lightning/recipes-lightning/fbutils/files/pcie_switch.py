#!/bin/bash

TMP_FILE=/tmp/pcie_switch_vendor
i2c_scan9=$(i2cdetect -y 9)
i2c_scan10=$(i2cdetect -y 10)

# Scan the i2cbus 9 and 10 and check for PLX switch. If they do not exist, the PCie used is PMC
if ([[ $i2c_scan9 == *"5d"* ]] && [[ $i2c_scan9 == *"5e"* ]] && [[ $i2c_scan9 == *"5f"* ]] \
  && [[ $i2c_scan10 == *"5d"* ]] && [[ $i2c_scan10 == *"5e"* ]] && [[ $i2c_scan10 == *"5f"* ]])
then
  echo -e "PLX" > $TMP_FILE
  echo -e "PLX switch selected"
else
  echo -e "PMC" > $TMP_FILE
  echo -e "PMC switch selected"
fi

exit 0
