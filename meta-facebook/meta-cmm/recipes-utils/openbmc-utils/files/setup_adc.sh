#!/bin/sh
#
# Copyright 2014-present Facebook. All Rights Reserved.
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

# channel 0: r1: 17.8K; r2:  2.2K; v2: 0mv
# channel 1: r1:  3.3K; r2:  1.0K; v2: 0mv
# channel 2: r1:  3.3K; r2:  2.2K; v2: 0mv
# channel 3: r1:  3.3K; r2:  3.3K; v2: 0mv
# channel 4: r1:  0.0K; r2:  1.0K; v2: 0mv
# channel 5: r1:  0.0K; r2:  1.0K; v2: 0mv
# channel 6: r1: 17.8K; r2:  2.2K; v2: 0mv
# channel 7: r1:  3.3K; r2:  1.0K; v2: 0mv
# channel 8-15: not used by cmm.

config_adc() {
    channel=$1
    r1=$2
    r2=$3
    v2=$4
    sysfs_adc_path="/sys/devices/platform/ast_adc.0"

    echo $r1 > ${sysfs_adc_path}/adc${channel}_r1
    echo $r2 > ${sysfs_adc_path}/adc${channel}_r2
    echo $v2 > ${sysfs_adc_path}/adc${channel}_v2
    echo 1 > ${sysfs_adc_path}/adc${channel}_en
}

bulk_setup_adc() {
    config_adc 0 178 22 0
    config_adc 1  33 10 0
    config_adc 2  33 22 0
    config_adc 3  33 33 0
    config_adc 4   0 10 0
    config_adc 5   0 10 0
    config_adc 6 178 22 0
    config_adc 7  33 10 0
}

#
# Passing R1, R2 and V2 values to kernel is only needed in kernel 4.1;
# In kernel 4.17 (or newer versions), voltage computation formulas are
# defined in lm_sensors config file.
#
KERNEL_VERSION=`uname -r`
if [[ ${KERNEL_VERSION} == 4.1.* ]]; then
    bulk_setup_adc
fi
