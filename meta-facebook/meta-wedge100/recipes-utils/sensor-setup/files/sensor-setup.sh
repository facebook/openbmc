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

SENSORS_CONF_PATH="/etc/sensors.d"

#
# Configure ADC channels: the step is only needed in kernel 4.1 because
# ADC channels have been configured in device tree in kernel 5.x.
#
config_adc_channels() {
    echo "Configure ADC Channels.."

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
}

#
# Some I2C devices may not get power until after power-on.sh, and it
# usually happens when the chassis is power cycled. Let's probe such devices
# again in this case.
#
fixup_i2c_devices() {
    mux_driver="pca954x"
    mux_device="7-0070"

    if [ ! -e "${SYSFS_I2C_DRIVERS}/${mux_driver}/${mux_device}" ]; then
        echo "Re-probe i2c switch $mux_device.."
        i2c_bind_driver "$mux_driver" "$mux_device"

        # A small delay to ensure all the i2c mux channels are ready.
        sleep 1
    fi
}

get_ltc4151_version() {
    bus="$1"
    addr="$2"

    i2cset -f -y "$bus" "$addr" 0x00 2> /dev/null
    usleep 50000
    i2cset -f -y "$bus" "$addr" 0x4D 2> /dev/null
    usleep 50000
    if output=$(i2cget -f -y "$bus" "$addr" 2> /dev/null); then
        echo "$output"
    else
        echo "-1"
    fi
}

#
# Adjust LTC4151 Rsense value depending on hardware version.
# Kernel driver use Rsense equal to 1 milliohm. We need to correct Rsense
# value, and all the value are from hardware team.
#
pem_sensor_calibration() {
    echo "Calibrate PEM sensors..."

    # mux-channel-1 (14-0056) and mux-channel-2 (15-0055)
    fru1_version="$(get_ltc4151_version 14 0x56)"
    fru2_version="$(get_ltc4151_version 15 0x55)"

    if [ "$((fru1_version))" -ge "$((0x02))" ] || \
       [ "$((fru2_version))" -ge "$((0x02))" ]; then
        rsense_val=1.55
    else
        rsense_val=1.35
    fi

    sed "s/rsense/$rsense_val/g" "${SENSORS_CONF_PATH}/custom/ltc4151.conf" \
        > "${SENSORS_CONF_PATH}/ltc4151.conf"
}

#
# Main entry starts here
#
if uname -r | grep "4\.1\.*" > /dev/null 2>&1; then
    config_adc_channels
fi

fixup_i2c_devices

pem_sensor_calibration
