# Copyright 2019-present Facebook. All Rights Reserved.
. /usr/local/bin/openbmc-utils.sh

SCMCPLD_SYSFS_DIR="$(i2c_device_sysfs_abspath 2-003e)"
SMBCPLD_SYSFS_DIR="$(i2c_device_sysfs_abspath 12-003e)"
PWR_USRV_SYSFS="${SCMCPLD_SYSFS_DIR}/com_exp_pwr_enable"
PWR_TH_RST_SYSFS="${SMBCPLD_SYSFS_DIR}/mac_reset_n"

wedge_iso_buf_enable() {
    # TODO, no isolation buffer
    return 0
}

wedge_iso_buf_disable() {
    # TODO, no isolation buffer
    return 0
}

wedge_is_us_on() {
    local val

    val=$(cat $PWR_USRV_SYSFS 2> /dev/null | head -n 1)
    if [ "$val" == "0x1" ]; then
        return 0            # powered on
    else
        return 1
    fi

    return 0
}

wedge_board_type() {
    echo 'WEDGE400'
}

wedge_slot_id() {
    printf "1\n"
}

wedge_board_rev() {
    local val0 val1 val2
    val0=$(gpio_get BMC_CPLD_BOARD_REV_ID0)
    val1=$(gpio_get BMC_CPLD_BOARD_REV_ID1)
    val2=$(gpio_get BMC_CPLD_BOARD_REV_ID2)
    echo $((val0 | (val1 << 1) | (val2 << 2)))
}

# Should we enable OOB interface or not
wedge_should_enable_oob() {
    # wedge400 uses BMC MAC since beginning
    return -1
}

wedge_power_on_board() {
    # wedge400 SCM power is controlled by SCMCPLD
    return 0
}
