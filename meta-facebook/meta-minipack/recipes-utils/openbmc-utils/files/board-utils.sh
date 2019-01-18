# Copyright 2018-present Facebook. All Rights Reserved.

#
# I2C paths of CPLDs
#
SCMCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 2-0035)
SMBCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 12-003e)
TOP_FCMCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 64-0033)
BOTTOM_FCMCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 72-0033)
LEFT_PDBCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 55-0060)
RIGHT_PDBCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 63-0060)

#
# I2C paths of FPGAs
#
IOBFPGA_SYSFS_DIR=$(i2c_device_sysfs_abspath 13-0035)
PIM1_DOMFPGA_SYSFS_DIR=$(i2c_device_sysfs_abspath 80-0060)
PIM2_DOMFPGA_SYSFS_DIR=$(i2c_device_sysfs_abspath 88-0060)
PIM3_DOMFPGA_SYSFS_DIR=$(i2c_device_sysfs_abspath 96-0060)
PIM4_DOMFPGA_SYSFS_DIR=$(i2c_device_sysfs_abspath 104-0060)
PIM5_DOMFPGA_SYSFS_DIR=$(i2c_device_sysfs_abspath 112-0060)
PIM6_DOMFPGA_SYSFS_DIR=$(i2c_device_sysfs_abspath 120-0060)
PIM7_DOMFPGA_SYSFS_DIR=$(i2c_device_sysfs_abspath 128-0060)
PIM8_DOMFPGA_SYSFS_DIR=$(i2c_device_sysfs_abspath 136-0060)

PWR_USRV_SYSFS="${SCMCPLD_SYSFS_DIR}/com_exp_pwr_enable"
PWR_TH_RST_SYSFS="${SMBCPLD_SYSFS_DIR}/cpld_mac_reset_n"

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
    echo 'MINIPACK'
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
    # minipack uses BMC MAC since beginning
    return -1
}

wedge_power_on_board() {
    # minipack SCM power is controlled by SCMCPLD
    return 0
}
