#!/bin/sh

cmd="ipmb-util 0x5 0x2c 0xb8 0xc8 0x57 0x01 0x00 0x01 0x00 0x00"

val=$($cmd)
ret=$?

if [ $((ret)) -ne 0 ]; then
    echo "command failed : $cmd"
    echo "  return value : $ret"
    exit 1
fi

# ref : Capture3 Intel ME Intel NM IPMI Interface
#       Intelz Intelligent Power Node Manager 3.0 External Interface Specification Using IPMI
#  NetFn 0x2E cmd 0xC8
# example response : 57 01 00 1E 00 0D 00 27 00 1E 00 FF FF FF FF EF 39 01 00 50
# [1:3]   - MFG ID
# [4:5]   - Current value
# [6:7]   - Minimum value
# [8:9]   - Maximum value
# [10:11] - Average value
# [12:15] - Timestamp
# [16:19]
# [20]    Domain Id | Policy

cur_power="0x$(echo "$val" | cut -d' ' -f5)$(echo "$val" | cut -d' ' -f4)"
min_power="0x$(echo "$val" | cut -d' ' -f7)$(echo "$val" | cut -d' ' -f6)"
max_power="0x$(echo "$val" | cut -d' ' -f9)$(echo "$val" | cut -d' ' -f8)"
avr_power="0x$(echo "$val" | cut -d' ' -f11)$(echo "$val" | cut -d' ' -f10)"

printf "INA230 Current power : %d Watts\n" "$cur_power"
printf "INA230 Minimum power : %d Watts\n" "$min_power"
printf "INA230 Maximum power : %d Watts\n" "$max_power"
printf "INA230 Average power : %d Watts\n" "$avr_power"