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
#

### BEGIN INIT INFO
# Provides:          sensor-setup
# Required-Start:    power-on
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Power on micro-server
### END INIT INFO

. /usr/local/bin/openbmc-utils.sh

if uname -r | grep "4\.1\.*" > /dev/null 2>&1; then
    # Eventually, this will be used to configure the various (mostly
    # i2c-based) sensors, once we have a kernel version that supports
    # doing this more dynamically.
    #
    # For now, we're using it to install the lm75 and pmbus module so that it
    # can detect the fourth temperature sensor, which is located
    # on the uServer, which doesn't get power until power-on executes.
    #
    # Similarly, the pmbus sensor seems to have an easier time of
    # detecting the NCP4200 buck converters after poweron.  This has not
    # been carefully explored.

    modprobe lm75
    modprobe pmbus

    modprobe ast_adc

    # channel 5: r1:  1.0K; r2:  1.0K; v2: 1000mv
    # channel 6: r1:  1.0K; r2:  1.0K; v2: 1000mv
    # channel 7: r1:  1.0K; r2:  1.0K; v2: 0mv
    # channel 8: r1:  1.0K; r2:  1.0K; v2: 0mv
    # channel 9: r1:  1.0K; r2:  1.0K; v2: 0mv
    ast_config_adc 5  1 1 1000
    ast_config_adc 6  1 1 1000
    ast_config_adc 7  1 1 0
    ast_config_adc 8  1 1 0
    ast_config_adc 9  1 1 0

    # Enable the ADC inputs;  adc5 - adc9 should be connected to
    # 1V, 1.03V, 5V, 3.3V, and 2.5V.
    echo 1 > /sys/devices/platform/ast_adc.0/adc5_en
    echo 1 > /sys/devices/platform/ast_adc.0/adc6_en
    echo 1 > /sys/devices/platform/ast_adc.0/adc7_en
    echo 1 > /sys/devices/platform/ast_adc.0/adc8_en
    echo 1 > /sys/devices/platform/ast_adc.0/adc9_en
fi
