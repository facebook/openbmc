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
    pim=$((i+2))
    # PIM numbered 2-9
    pim=$((i+2))
    if wedge_is_pim_present "$pim"; then
        pim_type=$(wedge_pim_type "$pim")
        if [[ "$pim_type" == "16q2" ]]; then
            echo "PIM${pim} has no DPM..."
        else
            # PIM 2-9, SMBUS 16-23
            bus_id="${pim_bus[$i]}"
            show_dpm_ver "PIM${pim}.:" "$bus_id" "$pim_dpm_addr"
        fi
    else
        echo "PIM${pim} not present... skipping."
    fi
done

show_isl_ver() {
  name="$1"
  bus="$2"
  addr="$3"

  mdl=$(i2cdump -f -y "${bus}" "${addr}" s 0x9a | grep 00: | awk '{print $5$4$3$2}')
  mfr=$(i2cdump -f -y "${bus}" "${addr}" s 0x9b | grep 00: | awk '{print $5$4$3$2}')
  rstr="SFT${mdl:1:5}${mdl:6:2}$(printf "%02d" "0x$mfr")"
  echo "$name: $rstr"
}

# ISL68226 Revision
show_isl_ver "SMB ISL68226" 3 0x62

# RAA228228 Revision
show_isl_ver "SMB RAA228228" 3 0x60

# PIM ISL Revision
pim_isl_addr='0x54'
for i in "${pim_index[@]}"
do
    pim=$((i+2))
    pim_type=$(wedge_pim_type "$pim")
    if [[ "$pim_type" == "8ddm" ]]; then
        bus_id="${pim_bus[$i]}"
        show_isl_ver "PIM${pim} ISL68224" "$bus_id" "$pim_isl_addr"
    elif [[ "$pim_type" == "16q2" ]]; then
        echo "PIM${pim} has no ISL... skipping."
    elif [[ "$pim_type" == "16q" ]]; then
        echo "PIM${pim} has no ISL... skipping."
    elif [[ "$pim_type" == "unplug" ]]; then
        echo "PIM${pim} not present... skipping."
    fi
done

echo "!!! Done."
