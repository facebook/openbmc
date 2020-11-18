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
pim_bus=(16 17 18 19 20 21 22 23)
if wedge_is_smb_p1; then
    # P1 has different PIM bus mapping
    pim_bus=(16 17 18 23 20 21 22 19)
fi
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

# ISL68226 Revision
isl_mdl=$(i2cdump -f -y 3 0x62 s 0x9a | grep 00: | awk '{print $5$4$3$2}')
isl_mfr=$(i2cdump -f -y 3 0x62 s 0x9b | grep 00: | awk '{print $5$4$3$2}')
isl_rstr="SFT${isl_mdl:1:5}${isl_mdl:6:2}$(printf "%02d" "0x$isl_mfr")"
echo "SMB ISL68226: $isl_rstr"

# RAA228228 Revision
raa_mdl=$(i2cdump -f -y 3 0x60 s 0x9a | grep 00: | awk '{print $5$4$3$2}')
raa_mfr=$(i2cdump -f -y 3 0x60 s 0x9b | grep 00: | awk '{print $5$4$3$2}')
raa_rstr="SFT${raa_mdl:1:5}${raa_mdl:6:2}$(printf "%02d" "0x$raa_mfr")"
echo "SMB RAA228228: $raa_rstr"

echo "!!! Done."
