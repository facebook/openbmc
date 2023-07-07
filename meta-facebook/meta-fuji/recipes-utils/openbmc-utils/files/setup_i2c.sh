#!/bin/bash
#
# Copyright 2020-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin
#shellcheck disable=SC1091
source /usr/local/bin/openbmc-utils.sh

# Sometimes, it has been observed that I2c devices, such as the PIM's UCD, 
# have the potential to lock the bus. This will render the entire i2c bus inoperable.
# To recover from such instances, reset the PIM i2c mux to restore the devices back to normal.
i2cset -y -f 12 0x3e 0x15 0x00
sleep 1
i2cset -y -f 12 0x3e 0x15 0xff

# Board Version
board_ver=$(wedge_board_rev)

# # Bus 0
i2c_device_add 0 0x1010 slave-mqueue #IPMB 0

# # Bus 1
i2c_device_add 1 0x40 xdpe132g5c #ISL68137 DC-DC core
i2c_device_add 1 0x53 mp2978
i2c_device_add 1 0x59 mp2978

# # Bus 2
i2c_device_add 2 0x35 scmcpld  #SCM CPLD

# # Bus 4
i2c_device_add 4 0x1010 slave-mqueue #IPMB 1
i2c_device_add 4 0x27 smb_debugcardcpld  # SMB DEBUGCARD CPLD

# # Bus 13
i2c_device_add 13 0x35 iobfpga #IOB FPGA

# # Bus 3
i2c_device_add 3 0x48 lm75     #LM75B_1# Thermal sensor
i2c_device_add 3 0x49 lm75     #LM75B_2# Thermal sensor
i2c_device_add 3 0x4a lm75     #LM75B_3# Thermal sensor

# # Bus 5
# # # SMB Power Sequence 1
# # # for MP
# # #   UCD90160    0x35
# # # for Respin
# # #   UCD90160A   0x66
# # #   UCD90160    0x68
# # #   UCD90124A   0x43
# # #   ADM1266     0x44
if i2c_detect_address 5 0x35; then
   i2c_device_add 5 0x35 ucd90160
   kv set smb_pwrseq_1_addr 0x35
elif i2c_detect_address 5 0x66; then
   i2c_device_add 5 0x66 ucd90160
   kv set smb_pwrseq_1_addr 0x66
elif i2c_detect_address 5 0x68; then
   i2c_device_add 5 0x68 ucd90160
   kv set smb_pwrseq_1_addr 0x68
elif i2c_detect_address 5 0x43; then
   i2c_device_add 5 0x43 ucd90124
   kv set smb_pwrseq_1_addr 0x43
elif i2c_detect_address 5 0x44; then
   i2c_device_add 5 0x44 adm1266
   kv set smb_pwrseq_1_addr 0x44
else
   echo "setup-i2c : not detect UCD90160 UCD9016A UCD90120 UCD90120A UCD90124 UCD90124A ADM1266" > /dev/kmsg
fi

# # # SMB Power Sequence 2
# # # for MP
# # #   UCD90160    0x36
# # # for Respin
# # #   UCD90160A   0x67
# # #   UCD90160    0x69
# # #   UCD90124A   0x46
# # #   ADM1266     0x47
if i2c_detect_address 5 0x36; then
   i2c_device_add 5 0x36 ucd90160
   kv set smb_pwrseq_2_addr 0x36
elif i2c_detect_address 5 0x67; then
   i2c_device_add 5 0x67 ucd90160
   kv set smb_pwrseq_2_addr 0x67
elif i2c_detect_address 5 0x69; then
   i2c_device_add 5 0x69 ucd90160
   kv set smb_pwrseq_2_addr 0x69
elif i2c_detect_address 5 0x46; then
   i2c_device_add 5 0x46 ucd90124
   kv set smb_pwrseq_2_addr 0x46
elif i2c_detect_address 5 0x47; then
   i2c_device_add 5 0x47 adm1266
   kv set smb_pwrseq_2_addr 0x47
else
   echo "setup-i2c : not detect UCD90160 UCD9016A UCD90120 UCD90120A UCD90124 UCD90124A ADM1266" > /dev/kmsg
fi

# Bus 8
i2c_device_add 8 0x51 24c64
i2c_device_add 8 0x4a lm75

# net_brcm driver only support DVT1 and later
if [ "$board_ver" != "BOARD_FUJI_EVT1" ] && \
[ "$board_ver" != "BOARD_FUJI_EVT2" ] && \
[ "$board_ver" != "BOARD_FUJI_EVT3" ]; then
    i2c_device_add 29 0x47 net_brcm
fi

# # i2c-mux PCA9548 0x70, channel 1, mux PCA9548 0x71
i2c_device_add 48 0x58 psu_driver
i2c_device_add 49 0x5a psu_driver
i2c_device_add 50 0x4c lm75     #SIM
i2c_device_add 50 0x52 24c64 	#SIM
i2c_device_add 51 0x48 tmp75
i2c_device_add 52 0x49 tmp75
i2c_device_add 54 0x21 pca9534	#PCA9534
if i2c_detect_address 55 0x60; then
    i2c_device_add 55 0x60 smb_pwrcpld 	#PDB-L
else
    i2c_device_add 53 0x60 smb_pwrcpld 	#PDB-L
fi

# # i2c-mux PCA9548 0x70, channel 2, mux PCA9548 0x72
i2c_device_add 56 0x58 psu_driver 	#PSU4
i2c_device_add 57 0x5a psu_driver 	#PSU3

i2c_device_add 59 0x48 tmp75
i2c_device_add 60 0x49 tmp75
i2c_device_add 62 0x21 pca9534
if i2c_detect_address 63 0x60; then
    i2c_device_add 63 0x60 smb_pwrcpld 	#PDB-R
else
    i2c_device_add 61 0x60 smb_pwrcpld  #PDB-R
fi

# # i2c-mux PCA9548 0x70, channel 3, mux PCA9548 0x76
i2c_device_add 64 0x33 fcbcpld #FCM CPLD
i2c_device_add 65 0x53 24c64
i2c_device_add 66 0x49 tmp75
i2c_device_add 66 0x48 tmp75
i2c_device_add 68 0x52 24c64    #fan 7 eeprom
i2c_device_add 69 0x52 24c64    #fan 5 eeprom
i2c_device_add 70 0x52 24c64    #fan 3 eeprom
i2c_device_add 71 0x52 24c64    #fan 1 eeprom

# # # FCM-T HSC
# # #    ADM1278  0x10
# # #    LM25066  0x44
if i2c_detect_address 67 0x10; then
   i2c_device_add 67 0x10 adm1278    # FCM ADM1278
   kv set smb_fcm_t_hsc_addr 0x10
elif i2c_detect_address 67 0x44; then
   i2c_device_add 67 0x44 lm25066    # FCM LM25066
   kv set smb_fcm_t_hsc_addr 0x44
else
   echo "setup-i2c : not detect ADM1278 LM25066 on FCM bus67" > /dev/kmsg
fi

# # i2c-mux PCA9548 0x70, channel 4, mux PCA9548 0x76
i2c_device_add 72 0x33 fcbcpld #FCM CPLD
i2c_device_add 73 0x53 24c64
i2c_device_add 74 0x49 tmp75
i2c_device_add 74 0x48 tmp75
i2c_device_add 76 0x52 24c64    #fan 8 eeprom
i2c_device_add 77 0x52 24c64    #fan 6 eeprom
i2c_device_add 78 0x52 24c64    #fan 4 eeprom
i2c_device_add 79 0x52 24c64    #fan 2 eeprom

# # # FCM-B HSC
# # #    ADM1278  0x10
# # #    LM25066  0x44
if i2c_detect_address 75 0x10; then
   i2c_device_add 75 0x10 adm1278    # FCM ADM1278
   kv set smb_fcm_b_hsc_addr 0x10
elif i2c_detect_address 75 0x44; then
   i2c_device_add 75 0x44 lm25066    # FCM LM25066
   kv set smb_fcm_b_hsc_addr 0x44
else
   echo "setup-i2c : not detect ADM1278 LM25066 on FCM bus75" > /dev/kmsg
fi

# # i2c-mux PCA9548 0x70, channel 5
i2c_device_add 28 0x50 24c02    #BMC54616S EEPROM

# # Bus 12
i2c_device_add 12 0x3e smb_syscpld     # SYSTEM CPLD

#
# Check if I2C devices are bound to drivers. A summary message (total #
# of devices and # of devices without drivers) will be dumped at the end
# of this function.
#
i2c_check_driver_binding "fix-binding"
