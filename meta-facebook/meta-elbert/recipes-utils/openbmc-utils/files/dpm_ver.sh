#!/bin/bash

# shellcheck disable=SC1091
source /usr/local/bin/openbmc-utils.sh

show_dpm_ver() {
  name=$1
  bus=$2
  addr=$3
  dump=$(i2cdump -f -y "${bus}" "${addr}" s 0x9e | grep -v abcdef)
  dpm_ver="${dump##* }"
  echo "$name" "$dpm_ver"
}
echo "!!!"
echo "!!! WARNING: Displaying DPM version may disrupt sensor reading !!!"
echo "!!!"
show_dpm_ver "SUP.:" 9 0x11
# ELBERTTODO SCD SUPPORT
# ELBERTTODO LC SUPPORT
echo "!!! Done."
