#!/bin/bash

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
SMB_TH4_I2C_EN_SYSFS="${SMBCPLD_SYSFS_DIR}/th4_i2c_bsc2_en"
SMB_FULL_POWER_SYSFS="${SMBCPLD_SYSFS_DIR}/smb_power_en"
PIM_SMB_MUX_RST="${SMBCPLD_SYSFS_DIR}/pim_smb_mux_rst"
PSU_SMB_MUX_RST="${SMBCPLD_SYSFS_DIR}/psu_smb_mux_rst"
SMB_P1_DETECT_PATH="/tmp/.smb_p1_board"
SMB_P2_DETECT_PATH="/tmp/.smb_p2_board"
DS4520_BUS=8
DS4520_DEV=$(( 0x50 ))
DS4520_IO0_REG=0xf2

wedge_iso_buf_enable() {
    return 0
}

wedge_iso_buf_disable() {
    return 0
}

wedge_is_bmc_personality() {
    io_reg_val=$(i2cget -y $DS4520_BUS $DS4520_DEV $DS4520_IO0_REG)
    if [ "$((io_reg_val & 0x1))" = "0" ]; then
        return 0
    else
        return 1
    fi
}

wedge_is_smb_p1() {
    # Check if we previously detected P1/P2 SMB
    if [ -f "$SMB_P1_DETECT_PATH" ]; then
        return 0
    elif [ -f "$SMB_P2_DETECT_PATH" ]; then
        return 1
    fi
    # Read SMB eeprom to determine if P1/P2
    if weutil smb | grep -q 'PCA014590[1|5]0[1|5|6]'; then
        touch "$SMB_P1_DETECT_PATH"
        return 0
    else
        touch "$SMB_P2_DETECT_PATH"
        return 1
    fi
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

wedge_is_pim_present() {
   # $1 -  pim range 2 - 9
   pim="$1"

   # PIM SMBus 16-23
   pim_index=(0 1 2 3 4 5 6 7)
   pim_bus=(16 17 18 19 20 21 22 23)
   if wedge_is_smb_p1; then
       # P1 has different PIM bus mapping
       pim_bus=(16 17 18 23 20 21 22 19)
   fi

   busId=${pim_bus[$((pim-2))]}
   pim_prsnt="$(head -n 1 "$SMBCPLD_SYSFS_DIR"/pim"$pim"_present)"
   if [ "$((pim_prsnt))" -eq 1 ]; then
      return 0
   prsnt_i2c="$(i2cget -f -y "$busId" 0x50 >/dev/null 2>&1 && echo 1 || echo 0)"
   elif [ "$((prsnt_i2c))" -eq 1 ]; then
      # PIM Eeprom found but PIM present bit is incorrect
      echo "PIM$pim present is false but eeprom detected."
      return 0
   fi
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
    old_rst_value=$(gpio_get PIM"${pim}"_FPGA_RESET_L keepdirection)
    if [ "$old_rst_value" -eq 1 ]; then
       # already powered on, skip
       return
    fi

    # Skip power_on if we are still powercycling
    if [ "$(head -n 1 "$SMBCPLD_SYSFS_DIR"/pim_reset)" != '0x0' ]; then
        logger pim_enable: skipping power_on_pim. PIM"$pim" is still powercycling
        return
    fi

    # Skip power_on if the FPGA is still being programmed
    pim_major=$(head -n 1 "$SMBCPLD_SYSFS_DIR"/pim"$pim"_fpga_rev_major)
    if [ "$((pim_major))" -eq 255 ]; then
        logger pim_enable: skipping power_on_pim. PIM"$pim" is fpga loading is not complete
        return
    fi

    fpga_done_value=$(gpio_get PIM"${pim}"_FPGA_DONE keepdirection)
    if [ "$fpga_done_value" -eq 0 ]; then
        logger pim_enable: skipping power_on_pim. PIM"$pim" is FPGA_DONE is not ready
        return
    fi

    sleep 0.5 # Give FPGA some time to finish initialization
    gpio_set PIM"${pim}"_FPGA_RESET_L 1  # FPGA out of reset
    sleep 0.1

    skip_pim_off_cache=$2
    pim_off_cache="/tmp/.pim${pim}_powered_off"
    if [ -z "${skip_pim_off_cache}" ] && [ -f "${pim_off_cache}" ]; then
       rm  "${pim_off_cache}"
    fi
    logger pim_enable: powered on PIM"${pim}"
}

power_off_pim() {
    pim=$1
    old_rst_value=$(gpio_get PIM"${pim}"_FPGA_RESET_L keepdirection)
    if [ "$old_rst_value" -eq 0 ]; then
       # already powered off, skip
       return
    fi

    gpio_set PIM"${pim}"_FPGA_RESET_L 0  # FPGA in reset
    sleep 0.1

    skip_pim_off_cache=$2
    pim_off_cache="/tmp/.pim${pim}_powered_off"
    if [ -z "${skip_pim_off_cache}" ]; then
       touch "${pim_off_cache}"
    fi
    logger pim_enable: powered off PIM"${pim}"
}

retry_command() {
    # Retry command up to $1 attempts
    local retries=$1
    shift

    local count=0
    until "$@"; do
        exit=$?
        count=$((count+1))
        if [ "$count" -lt "$retries" ]; then
            echo "Attempt $count/$retries failed with $exit, retrying..."
        else
            echo "Retry $count/$retries failed with $exit, no more retries left"
            return $exit
        fi
    done
    return 0
}

FWUPGRADE_PIDFILE="/var/run/firmware_upgrade.pid"
check_fwupgrade_running()
{
    exec 200>$FWUPGRADE_PIDFILE
    flock -n 200 || (echo "Another FW upgrade is running" && exit 1)
    ret=$?
    if [ $ret -eq 1 ]; then
      exit 1
    fi
    pid=$$
    echo $pid 1>&200
}
