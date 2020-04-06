#!/bin/sh

# Copyright 2020-present Facebook. All Rights Reserved.

SUPCPLD_SYSFS_DIR="/sys/bus/i2c/devices/12-0043"
SUP_PWR_ON_SYSFS="${SUPCPLD_SYSFS_DIR}/cpu_control"

# shellcheck disable=SC2034
PWR_SYSTEM_SYSFS="${SUPCPLD_SYSFS_DIR}/chassis_power_cycle"

wedge_iso_buf_enable() {
    return 0
}

wedge_iso_buf_disable() {
    return 0
}

wedge_is_us_on() {
    isWedgeOn=$(head -n 1 $SUP_PWR_ON_SYSFS 2> /dev/null)
    if [ -z "$isWedgeOn" ]; then
        return "$default"
    elif [ "$isWedgeOn" = "0x1" ]; then
        # Powered On
        return 0
    else
        return 1
    fi
}

wedge_board_type() {
    echo 'ELBERT'
}

wedge_slot_id() {
    printf "1\n"
}

wedge_board_rev() {
    # Assume P1
    return 1
}

wedge_should_enable_oob() {
    return 1
}

wedge_power_off_asic() {
   # ELBERTTODO SCDCPLD
   echo 'Not Implemented power_off_asic!'
}

wedge_power_on_asic() {
   # ELBERTTODO SCDCPLD
   echo 'Not Implemented power_on_asic!'
}

wedge_power_on_board() {
    echo 1 > $SUP_PWR_ON_SYSFS
    wedge_power_off_asic
    wedge_power_on_asic
}

wedge_power_off_board() {
    # Order matters here
    wedge_power_off_asic
    echo 0 > $SUP_PWR_ON_SYSFS
    # Leave SCD on
}
