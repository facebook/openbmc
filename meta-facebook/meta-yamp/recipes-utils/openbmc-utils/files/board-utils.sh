# Copyright 2018-present Facebook. All Rights Reserved.

SUPCPLD_SYSFS_DIR="/sys/class/i2c-dev/i2c-12/device/12-0043"
SCDCPLD_SYSFS_DIR="/sys/class/i2c-dev/i2c-4/device/4-0023"
SUP_PWR_ON_SYSFS="${SUPCPLD_SYSFS_DIR}/cpu_control"
PWR_SYSTEM_SYSFS="${SUPCPLD_SYSFS_DIR}/chassis_power_cycle"
SCD_TH3_RST_ON_SYSFS="${SCDCPLD_SYSFS_DIR}/th3_reset"
SCD_TH3_PCI_RST_ON_SYSFS="${SCDCPLD_SYSFS_DIR}/th3_pci_reset"
SCD_FULL_POWER_SYSFS="${SCDCPLD_SYSFS_DIR}/scd_power_en"
LC_SMB_MUX_RST="${SCDCPLD_SYSFS_DIR}/lc_smb_mux_rst"

wedge_iso_buf_enable() {
    return 0
}

wedge_iso_buf_disable() {
    return 0
}

wedge_is_us_on() {
    local val
    val=$(cat $SUP_PWR_ON_SYSFS 2> /dev/null | head -n 1)
    if [ -z "$val" ]; then
        return $default
    elif [ "$val" == "0x1" ]; then
        return 0            # powered on
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
    # assume P2
    return 2
}

# Should we enable OOB interface or not
wedge_should_enable_oob() {
    # YAMP uses BMC MAC since beginning
    return -1
}

wedge_power_off_asic() {
    # It is not exact power off.
    # Instead, the ASIC is kept in reset in this case.
    echo 1 > $SCD_TH3_RST_ON_SYSFS
    sleep 1
    echo 1 > $SCD_TH3_PCI_RST_ON_SYSFS
}

wedge_power_on_asic() {
    # Order matters here
    echo 0 > $SCD_TH3_RST_ON_SYSFS
    usleep 250000
    echo 0 > $SCD_TH3_PCI_RST_ON_SYSFS
}

wedge_power_on_board() {
    # Order matters here
    echo 1 > $SUP_PWR_ON_SYSFS
    sleep 1
    wedge_power_off_asic
    sleep 1
    wedge_power_on_asic
    sleep 1
}

wedge_power_off_board() {
    # Order matters here
    wedge_power_off_asic
    sleep 1
    echo 0 > $SUP_PWR_ON_SYSFS
    # we leave SCD full power on in this case
}
