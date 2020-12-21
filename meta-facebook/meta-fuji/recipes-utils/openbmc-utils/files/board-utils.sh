#!/bin/bash
#
# Copyright 2020-present Facebook. All Rights Reserved.
#
# I2C paths of CPLDs
#
#shellcheck disable=SC2034
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

wedge_board_rev() {
    local val0 val1 val2
    val0=$(gpio_get_value BMC_CPLD_BOARD_REV_ID0)
    val1=$(gpio_get_value BMC_CPLD_BOARD_REV_ID1)
    val2=$(gpio_get_value BMC_CPLD_BOARD_REV_ID2)
    echo $((val0 | (val1 << 1) | (val2 << 2)))
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
