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
echo "!!! WARNING: Displaying Digital Power Manager (DPM) version may disrupt sensor reading !!!"
echo "!!!"

scm_dpm_bus='9'
scm_dpm_addr='0x11'
smb_dpm_bus='3'
smb_dpm_addr='0x4e'

show_dpm_ver "SCM.:" "$scm_dpm_bus" "$scm_dpm_addr"
show_dpm_ver "SMB.:" "$smb_dpm_bus" "$smb_dpm_addr"

# PIM DPM
pim_index=(0 1 2 3 4 5 6 7)
# This is a map between PIM2-9 to associated i2c BUS
# Currently, this doesn't map in order between PIM2-9 to bus 16-23
pim_bus=(16 17 18 23 20 21 22 19)
pim_dpm_addr='0x4e'

for i in "${pim_index[@]}"
do
    # PIM numbered 2-9
    pim=$((i+2))
    pim_prsnt="$(head -n 1 "$SMBCPLD_SYSFS_DIR"/pim"$pim"_present)"
    if [ "$((pim_prsnt))" -eq 1 ]; then
        # PIM 2-9, SMBUS 16-23
        bus_id="${pim_bus[$i]}"
        show_dpm_ver "PIM${pim}.:" "$bus_id" "$pim_dpm_addr"
    else
        echo "PIM${pim} not present... skipping."
    fi
done
echo "!!! Done."
