#!/bin/sh

# Copyright 2020-present Facebook. All Rights Reserved.

# shellcheck disable=SC2034

SCMCPLD_SYSFS_DIR="$(i2c_device_sysfs_abspath 12-0043)"
SMBCPLD_SYSFS_DIR="$(i2c_device_sysfs_abspath 4-0023)"
FANCPLD_SYSFS_DIR="$(i2c_device_sysfs_abspath 6-0060)"
SCM_PWR_ON_SYSFS="${SCMCPLD_SYSFS_DIR}/cpu_control"
SCM_SMB_POWERGOOD_SYSFS="${SCMCPLD_SYSFS_DIR}/switchcard_powergood"
PWR_SYSTEM_SYSFS="${SCMCPLD_SYSFS_DIR}/chassis_power_cycle"
SMB_TH4_RST_ON_SYSFS="${SMBCPLD_SYSFS_DIR}/th4_reset"
SMB_TH4_PCI_RST_ON_SYSFS="${SMBCPLD_SYSFS_DIR}/th4_pci_reset"
SMB_FULL_POWER_SYSFS="${SMBCPLD_SYSFS_DIR}/smb_power_en"
PIM_SMB_MUX_RST="${SMBCPLD_SYSFS_DIR}/pim_smb_mux_rst"
PSU_SMB_MUX_RST="${SMBCPLD_SYSFS_DIR}/psu_smb_mux_rst"

wedge_iso_buf_enable() {
    return 0
}

wedge_iso_buf_disable() {
    return 0
}

wedge_is_us_on() {
    isWedgeOn="$(head -n 1 "$SCM_PWR_ON_SYSFS" 2> /dev/null)"
    if [ -z "$isWedgeOn" ]; then
        return 1
    elif [ "$isWedgeOn" = "0x1" ]; then
        return 0 # uServer is on
    else
        return 1
    fi
}

wedge_is_smb_powergood() {
    isPowerGood="$(head -n 1 "$SCM_SMB_POWERGOOD_SYSFS" 2> /dev/null)"
    if [ -z "$isPowerGood" ]; then
        return 1
    elif [ "$isPowerGood" = "0x1" ]; then
        return 0 # Power is good
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
    # ASIC put into reset
    echo 1 > "$SMB_TH4_RST_ON_SYSFS"
    echo 1 > "$SMB_TH4_PCI_RST_ON_SYSFS"
    # Turn off power to ASIC
    echo 0 > "$SMB_FULL_POWER_SYSFS"
}

wedge_power_on_asic() {
    # Enable power to asic
    echo 1 > "$SMB_FULL_POWER_SYSFS"
    sleep 1

    if wedge_is_smb_powergood; then
        echo "Switchcard power is good..."
    else
        echo "FATAL ERROR: Switchcard power signal is NOT good."
    fi

    # Take ASIC out of reset. Order matters here
    echo 0 > "$SMB_TH4_RST_ON_SYSFS"
    usleep 250000
    echo 0 > "$SMB_TH4_PCI_RST_ON_SYSFS"
}

wedge_power_on_board() {
    wedge_power_off_asic
    wedge_power_on_asic
    echo 1 > "$SCM_PWR_ON_SYSFS"
}

wedge_power_off_board() {
    wedge_power_off_asic
    echo 0 > "$SCM_PWR_ON_SYSFS"
}

power_on_pim() {
    pim=$1
    old_value=$(gpio_get PIM"${pim}"_FULL_POWER_EN keepdirection)
    if [ "$old_value" -eq 1 ]; then
       logger pim_enable: PIM"${pim}" already powered on
       # already powered on, skip
       return
    fi
    gpio_set PIM"${pim}"_FULL_POWER_EN 1 # full power on
    gpio_set PIM"${pim}"_FPGA_RESET_L 1  # FPGA out of reset
    logger pim_enable: powered on PIM"${pim}"
}
