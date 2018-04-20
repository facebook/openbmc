# Copyright 2018-present Facebook. All Rights Reserved.

SUPCPLD_SYSFS_DIR="/sys/class/i2c-dev/i2c-12/device/12-0043"
SCDCPLD_SYSFS_DIR="/sys/class/i2c-dev/i2c-4/device/4-0023"
SUP_PWR_ON_GPIO="SUP_PWR_ON"

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
    # We set the GPIO first before enabling BMC control.
    # This is because when BMC is rebooted, the GPIO could go back to low.
    # If we enable BMC control then, the CPU will be powered off.
    # In this case, make sure the GPIO is set correctly before enabling
    # BMC control.
    gpio_set $SUP_PWR_ON_GPIO 1
    enable_bmc_control
    sleep 1
    disable_bmc_control
    return 0
}

wedge_power_off_board() {
    gpio_set $SUP_PWR_ON_GPIO 0
    enable_bmc_control
    sleep 1
    disable_bmc_control
    return 0
}

enable_bmc_control() {
    echo 0x1 > ${SUPCPLD_SYSFS_DIR}/cpu_control
}

disable_bmc_control() {
    echo 0x0 > ${SUPCPLD_SYSFS_DIR}/cpu_control
}
