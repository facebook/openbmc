# Copyright 2018-present Facebook. All Rights Reserved.

SCMCPLD_SYSFS_DIR="/sys/class/i2c-adapter/i2c-2/2-0035"
SMBCPLD_SYSFS_DIR="/sys/class/i2c-adapter/i2c-12/12-003e"
SUP_PWR_ON_GPIO="SUP_PWR_ON"
SUP_PWR_ON_DIR=

wedge_iso_buf_enable() {
    return 0
}

wedge_iso_buf_disable() {
    return 0
}

wedge_is_us_on() {
    local val

    val=$(gpio_get $SUP_PWR_ON_GPIO keepdirection)
    if [ "$val" == "1" ]; then
        return 0                # powered on
    else
        return 1
    fi
}

wedge_board_type() {
    echo 'YAMP'
}

wedge_slot_id() {
    printf "1\n"
}

wedge_board_rev() {
    # assume P1
    return 1
}

# Should we enable OOB interface or not
wedge_should_enable_oob() {
    # YAMP uses BMC MAC since beginning
    return -1
}

wedge_power_on_board() {
    gpio_set $SUP_PWR_ON_GPIO 1
    return 0
}

wedge_power_off_board() {
    gpio_set $SUP_PWR_ON_GPIO 0
    return 0
}
