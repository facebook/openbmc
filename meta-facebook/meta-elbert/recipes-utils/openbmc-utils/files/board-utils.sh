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
UCD_SECURITY_REG=0xf1
WRITE_PROTECT_REG=0x10

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

wedge_pim_bus() {
   # $1 -  pim range 2 - 9
   pim="$1"

   # PIM SMBus 16-23
   pim_index=(0 1 2 3 4 5 6 7)
   pim_bus=(16 17 18 19 20 21 22 23)
   if wedge_is_smb_p1; then
       # P1 has different PIM bus mapping
       pim_bus=(16 17 18 23 20 21 22 19)
   fi
   echo "${pim_bus[$((pim-2))]}"
}

wedge_is_pim_present() {
   # $1 -  pim range 2 - 9
   pim="$1"

   busId=$(wedge_pim_bus "$pim")
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

wedge_pim_type() {
   pim="$1"

   busId=$(wedge_pim_bus "$pim")
   pim_type=$(/usr/bin/kv get pim"$pim"_type)
   echo "$pim_type"
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

maybe_set_security_bitmask() {
    bus="$1"
    dev="$2"
    model="$3"

    if [[ "$model" == *"UCD9090"* ]]
    then
        # A 0 bit indicates the command is not write protected.
        # Each bit represents a command code where the codes
        # increment left to right. eg. byte 0, bit 7 = 00,
        # byte 0, bit 6 = 01, etc.
        #
        # The following commands are allowed:
        #   00       - PAGE
        #   03       - CLEAR_FAULTS
        #   D7       - RUN_TIME_CLOCK
        #   EA-ED,EF - FAULT commands
        #   F0       - EXECUTE_FLASH
        #   F1       - SECURITY
        #   F2       - SECURITY_BIT_MASK
        #   FA       - GPIO SELECT
        #   FB       - GPIO CONFIG
        #   FC       - MISC CONFIG
        i2cset -f -y "$bus" "$dev" 0xf2 \
           0x6f 0xff 0xff 0xff 0xff 0xff 0xff 0xff \
           0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff \
           0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff \
           0xff 0xff 0xfe 0xff 0xff 0xc2 0x1f 0xc7 s
    #else
    #    Model does not support security bitmask, so treat as no-op
    fi
}

enable_power_security_mode() {
    bus="$1"
    dev="$2"
    model="$3"

    # Some UCD models support a security bitmask
    maybe_set_security_bitmask "$bus" "$dev" "$model"

    locked="$(printf "%d" "$(i2cget -f -y "$bus" "$dev" "$UCD_SECURITY_REG" s | awk '{print $(NF)}')")"
    if [ "$locked" -eq 0 ]; then
        # Write a new 6-byte password, enabling security
        i2cset -f -y "$bus" "$dev" "$UCD_SECURITY_REG" 0x06 0x31 0x32 0x33 0x34 0x35 0x36 i
        locked="$(printf "%d" "$(i2cget -f -y "$bus" "$dev" "$UCD_SECURITY_REG" s | awk '{print $(NF)}')")"
        if [ "$locked" -eq 0 ]; then
            echo "Failed to lock $model $bus-00$dev"
            return 1
        fi
    else
        echo "UCD bus $bus device $dev already locked!"
        return 0
    fi
}

disable_power_security_mode() {
    bus="$1"
    dev="$2"
    model="$3"

    # If locked, unlock it
    locked="$(printf "%d" "$(i2cget -f -y "$bus" "$dev" "$UCD_SECURITY_REG" s | awk '{print $(NF)}')")"
    if [ "$locked" -eq 0 ]; then
        echo "UCD bus $bus device $dev already unlocked!"
        return 0
    else
        # If security is enabled, disable security by writing the correct password
        i2cset -f -y "$bus" "$dev" "$UCD_SECURITY_REG" 0x06 0x31 0x32 0x33 0x34 0x35 0x36 i
        # Make the security disable permanent. Otherwise a subsequent STORE_DEFAULT_ALL command
        # will persist the security setting.
        i2cset -f -y "$bus" "$dev" "$UCD_SECURITY_REG" 0x06 0xff 0xff 0xff 0xff 0xff 0xff i
        locked="$(printf "%d" "$(i2cget -f -y "$bus" "$dev" "$UCD_SECURITY_REG" s | awk '{print $(NF)}')")"
        if [ "$locked" -eq 0 ]; then
            return 0
        else
           echo "Failed to lock $model $bus-00$dev"
           return 1
        fi
    fi
}

enable_tps546d24_wp() {
    bus="$1"
    dev="$2"

    # Disable all writes except for WRITE_PROTECT and STORE_USER_ALL
    i2cset -f -y "$bus" "$dev" "$WRITE_PROTECT_REG" 0x80
}

disable_tps546d24_wp() {
    bus="$1"
    dev="$2"

    # Enable all writes
    i2cset -f -y "$bus" "$dev" "$WRITE_PROTECT_REG" 0x0
}

# Not all PIMs have the ISL68224. Calling this for a PIM
# without one is a no-op.
maybe_enable_isl_wp() {
    bus="$1"
    dev="$2"

    devhex=$(printf "%02x" "$dev")
    if [ ! -e /sys/bus/i2c/devices/"$bus"-00"$devhex" ]
    then
        return 0
    fi
    
    # Disable all writes except for WRITE_PROTECT, OPERATION, and PAGE
    i2cset -f -y "$bus" "$dev" "$WRITE_PROTECT_REG" 0x40
}

maybe_disable_isl_wp() {
    bus="$1"
    dev="$2"

    devhex=$(printf "%02x" "$dev")
    if [ ! -e /sys/bus/i2c/devices/"$bus"-00"$devhex" ]
    then
        return 0
    fi
    
    # Enable all writes
    i2cset -f -y "$bus" "$dev" 0x10 0x0
}

enable_switchcard_power_security() {
    retry_command 3 enable_power_security_mode 3 0x4e "UCD90160B"
    retry_command 3 maybe_enable_isl_wp 3 0x60
    retry_command 3 maybe_enable_isl_wp 3 0x62
}

disable_switchcard_power_security() {
    retry_command 3 disable_power_security_mode 3 0x4e "UCD90160B"
    retry_command 3 maybe_disable_isl_wp 3 0x60
    retry_command 3 maybe_disable_isl_wp 3 0x62
}

enable_psu_wp() {
    bus="$1"
    dev="$2"

    # Disable all writes except for WRITE_PROTECT, OPERATION, and PAGE
    i2cset -f -y "$bus" "$dev" "$WRITE_PROTECT_REG" 0x40
}

disable_psu_wp() {
    bus="$1"
    dev="$2"

    # Enable all writes
    i2cset -f -y "$bus" "$dev" "$WRITE_PROTECT_REG" 0x0
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
    bus=$(wedge_pim_bus "$pim")
    dev="0x4e"
    gpio_set PIM"${pim}"_FPGA_RESET_L 1  # FPGA out of reset

    logger pim_enable: Locking PIM"$pim" UCD9090B security
    retry_command 3 enable_power_security_mode "$bus" "$dev" "UCD9090B"

    logger pim_enable: Enable WRITE_PROTECT on PIM"$pim" TPS/ISL chips
    retry_command 3 enable_tps546d24_wp "$bus" 0x16
    retry_command 3 enable_tps546d24_wp "$bus" 0x18
    retry_command 3 maybe_enable_isl_wp "$bus" 0x54

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

    bus=$(wedge_pim_bus "$pim")
    dev="0x4e"
    gpio_set PIM"${pim}"_FPGA_RESET_L 0  # FPGA in of reset
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
            msg="Attempt $count/$retries failed with $exit, retrying..."
            echo "$msg"
            logger retry_command: "$msg"
        else
            msg="Retry $count/$retries failed with $exit, no more retries left"
            echo "$msg"
            logger retry_command: "$msg"
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
