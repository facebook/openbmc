#!/bin/bash
#
# Copyright 2020-present Facebook. All Rights Reserved.
#
# I2C paths of CPLDs
#
#shellcheck disable=SC2034
#shellcheck disable=SC1091
. /usr/local/bin/i2c-utils.sh

SCMCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 2-0035)
SMBCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 12-003e)
TOP_FCMCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 64-0033)
BOTTOM_FCMCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 72-0033)

#
# FUJI PDB has different I2C address depending on its version, but
# trying I2C address everytime we need to access, is not efficient and
# it also breaks provisioning logic that runs with bash -x option.
# So we will detect it once one the fly, and save the info in /tmp dir.
#
_PDB_L_OLD_CACHE=/tmp/pdb_l_old
_PDB_L_NEW_CACHE=/tmp/pdb_l_new
_PDB_R_OLD_CACHE=/tmp/pdb_r_old
_PDB_R_NEW_CACHE=/tmp/pdb_r_new
_PDB_L_OLD_ADDR=$(i2c_device_sysfs_abspath 53-0060)
_PDB_L_NEW_ADDR=$(i2c_device_sysfs_abspath 55-0060)
_PDB_R_OLD_ADDR=$(i2c_device_sysfs_abspath 61-0060)
_PDB_R_NEW_ADDR=$(i2c_device_sysfs_abspath 63-0060)

#
# If previously Left PDB was detected, used that cached info,
# otherwise try I2C to detect the Left PDB I2C address
#
if [ -f "$_PDB_L_NEW_CACHE" ]; then
  LEFT_PDBCPLD_SYSFS_DIR=$_PDB_L_NEW_ADDR
elif [ -f "$_PDB_L_OLD_CACHE" ]; then
  LEFT_PDBCPLD_SYSFS_DIR=$_PDB_L_OLD_ADDR
elif i2cget -y -f 55 0x60 > /dev/null;then
  LEFT_PDBCPLD_SYSFS_DIR=$_PDB_L_NEW_ADDR
  touch $_PDB_L_NEW_CACHE
else
  LEFT_PDBCPLD_SYSFS_DIR=$_PDB_L_OLD_ADDR
  touch $_PDB_L_OLD_CACHE
fi

#
# If previously Right PDB was detected, used that cached info,
# otherwise try I2C to detect the Right PDB I2C address
#
if [ -f "$_PDB_R_NEW_CACHE" ]; then
  RIGHT_PDBCPLD_SYSFS_DIR=$_PDB_R_NEW_ADDR
elif [ -f "$_PDB_R_OLD_CACHE" ]; then
  RIGHT_PDBCPLD_SYSFS_DIR=$_PDB_R_OLD_ADDR
elif i2cget -y -f 63 0x60 > /dev/null;then
  RIGHT_PDBCPLD_SYSFS_DIR=$_PDB_R_NEW_ADDR
  touch $_PDB_R_NEW_CACHE
else
  RIGHT_PDBCPLD_SYSFS_DIR=$_PDB_R_OLD_ADDR
  touch $_PDB_R_OLD_CACHE
fi

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
PWR_USRV_FORCE_OFF="${SCMCPLD_SYSFS_DIR}/com_exp_pwr_force_off"
PIM_RST_SYSFS="${SMBCPLD_SYSFS_DIR}"

# 48V DC PSU P/N
DELTA_48V="0x45 0x43 0x44 0x32 0x35 0x30 0x31 0x30 0x30 0x31 0x35"
LITEON_48V="0x44 0x44 0x2d 0x32 0x31 0x35 0x32 0x2d 0x32 0x4c"

i2c_detect_address() {
   bus="$1"
   addr="$2"
   reg="$3"
      
   retries=5
   retry=0

   while [ "$retry" -lt "$retries" ]; do
      if eval i2cget -y -f "$bus" "$addr" "$reg" &> /dev/null; then
         return 0
      fi
      usleep 50000
      retry=$((retry + 1))
   done
   
   echo "setup-i2c : i2c_detect not found $bus - $addr" > /dev/kmsg
   return 1
}

wedge_is_us_on() {
    local val0 val1

    val0=$(head -n 1 < "$PWR_USRV_SYSFS" 2> /dev/null)
    val1=$(head -n 1 < "$PWR_USRV_FORCE_OFF" 2> /dev/null)

    if [ "$val0" == "0x1" ] && [ "$val1" == "0x1" ] ; then
        return 0            # powered on
    else
        return 1
    fi

    return 0
}

wedge_board_rev() {
    board_ver=$(i2cget -f -y 12 0x3e 0x0 | tr -d '\n' ; \
                i2cget -f -y 13 0x35 0x3 | cut -d'x' -f2)
    case $((board_ver)) in
        $((0x0043)))  echo "BOARD_FUJI_EVT1"              ;;
        $((0x0041)))  echo "BOARD_FUJI_EVT2"              ;;
        $((0x0042)))  echo "BOARD_FUJI_EVT3"              ;;
        $((0x0343)))  echo "BOARD_FUJI_DVT1/DVT2"         ;;
        $((0x0545)))  echo "BOARD_FUJI_PVT1/PVT2"         ;;
        $((0x0747)))
          # MP0-SKU1, MP1-SKU7 have same value,
          # need to indicate from UCD90160A address 
          rev="$(kv get smb_pwrseq_1_addr)"
          if [ "$rev" = "0x35" ]; then
            echo "BOARD_FUJI_MP0-SKU1"
          elif [ "$rev" = "0x66" ]; then
            echo "BOARD_FUJI_MP1-SKU7"
          else
            echo "BOARD_FUJI_UNDEFINED_${board_ver}"
          fi
          ;;
        $((0x2747)))  
          # MP0-SKU2, MP1-SKU6 have same value,
          # need to indicate from UCD90160 address
          rev="$(kv get smb_pwrseq_1_addr)"
          if [ "$rev" = "0x35" ]; then
            echo "BOARD_FUJI_MP0-SKU2"
          elif [ "$rev" = "0x68" ]; then
            echo "BOARD_FUJI_MP1-SKU6"
          else
            echo "BOARD_FUJI_UNDEFINED_${board_ver}"
          fi
          ;;
        $((0x1747)))  echo "BOARD_FUJI_MP1-SKU1"          ;;
        $((0x3747)))  echo "BOARD_FUJI_MP1-SKU2"          ;;
        $((0x0757)))  echo "BOARD_FUJI_MP1-SKU3"          ;;
        $((0x0767)))  echo "BOARD_FUJI_MP1-SKU4"          ;;
        $((0x0777)))  echo "BOARD_FUJI_MP1-SKU5"          ;;
        $((0x1040)))  echo "BOARD_FUJI_MP2-SKU1"          ;;
        $((0x1044)))  echo "BOARD_FUJI_MP2-SKU2"          ;;
        $((0x1042)))  echo "BOARD_FUJI_MP2-SKU3"          ;;
        $((0x1046)))  echo "BOARD_FUJI_MP2-SKU4"          ;;
        *)  # New board version need to be added here
            echo "BOARD_FUJI_UNDEFINED_${board_ver}"
            ;;
    esac
}

wedge_power_supply_type() {

    if i2cget -y -f 48 0x58 0x9a s &> /dev/null;then    # PSU1
        is_psu1_48v=$(i2cget -y -f 48 0x58 0x9a s &)
    elif i2cget -y -f 49 0x5a 0x9a s &> /dev/null;then  # PSU2
        is_psu2_48v=$(i2cget -y -f 49 0x5a 0x9a s &)
    elif i2cget -y -f 56 0x58 0x9a s &> /dev/null;then  # PSU3
        is_psu3_48v=$(i2cget -y -f 56 0x58 0x9a s &)
    elif i2cget -y -f 57 0x5a 0x9a s &> /dev/null;then  # PSU4
        is_psu4_48v=$(i2cget -y -f 57 0x5a 0x9a s &)
    fi

    # Detect 48V DC PSU
    if [[ "$is_psu1_48v" = "$DELTA_48V" ||
        "$is_psu1_48v" = "$LITEON_48V" ||
        "$is_psu2_48v" = "$DELTA_48V" ||
        "$is_psu2_48v" = "$LITEON_48V" ||
        "$is_psu3_48v" = "$DELTA_48V" ||
        "$is_psu3_48v" = "$LITEON_48V" ||
        "$is_psu4_48v" = "$DELTA_48V" ||
        "$is_psu4_48v" = "$LITEON_48V" ]];then
            power_type="PSU48"
    else
            power_type="PSU"
    fi
    echo $power_type
}

wedge_prepare_cpld_update() {
    echo "Stop fscd service."
    if ! systemctl stop fscd ; then
        echo "Error: FSCD fails to exit properly!"
        return 1
    fi

    echo "Disable watchdog."
    if ! /usr/local/bin/wdtcli stop ; then
        echo "Error: Failed to disable watchdog!"
        return 1
    fi

    echo "Set fan speed to 40%."
    set_fan_speed.sh 40
    
    return 0
}

wdt_set_timeout() {
    echo "Stop fscd service."
    /bin/systemctl stop fscd

    echo "Setting WTD1 timeout value to $1 seconds."
    /usr/local/bin/wdtcli set-timeout "$1"

    echo "Restore fscd service."
    /bin/systemctl start fscd
}
