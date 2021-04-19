# /usr/bin/env python
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
# Free Software Foundation, Inc.
# 51 Franklin Street, Fifth Floor
# Boston, MA 02110-1301 USA
#

LTC4282_SYSFS_NODES = [
    "in1_input",
    "in2_input",
    "curr1_input",
    "power1_input",
    "temp1_input",
    "fault_ov",
    "fault_uv",
    "fault_oc",
    "fault_power",
    "on_fault",
    "fault_fet_short",
    "fault_fet_bad",
    "eeprom_done",
    "power_alarm_high",
    "power_alarm_low",
    "vsense_alarm_high",
    "vsense_alarm_low",
    "vsource_alarm_high",
    "vsource_alarm_low",
    "gpio_alarm_high",
    "gpio_alarm_low",
    "on_status",
    "fet_bad",
    "fet_short",
    "on_pin_status",
    "power_good",
    "oc_status",
    "uv_status",
    "ov_status",
    "gpio3_status",
    "gpio2_status",
    "gpio1_status",
    "alert_status",
    "eeprom_busy",
    "adc_idle",
    "ticker_overflow",
    "meter_overflow",
    "in3_input",
    "in4_input",
]

MAX6615_SYSFS_NODES = [
    "temp1_input",
    "temp2_input",
    "temp1_emergency",
    "temp2_emergency",
    "temp1_emergency_alarm",
    "temp2_emergency_alarm",
    "pwm1",
    "pwm2",
    "pwm1_input",
    "pwm2_input",
    "pwm1_start",
    "pwm2_start",
    "pwm1_max",
    "pwm2_max",
    "pwm1_step",
    "pwm2_step",
    "pwm_freq",
    "fan1_input",
    "fan2_input",
    "fan1_limit",
    "fan2_limit",
    "fan1_fault",
    "fan2_fault",
]
