#!/bin/sh
#
# Copyright 2015-present Facebook. All Rights Reserved.
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
. /usr/local/fbpackages/utils/ast-functions
PWM_DIR=/sys/devices/platform/ast_pwm_tacho.0

set -e
kernel_ver=$(get_kernel_ver)
if [ $kernel_ver == 4 ]; then
    # The PWM frequency is
    
    #   clk_source / ((2 ^ division_) * (2 * division_l) * (unit + 1))
    #
    # Our clk_source is 24Mhz.  4-pin fans are generally supposed to be driven with
    # a 25Khz PWM control signal.  Therefore we want the divisor to equal 960.
    #
    # We also want the unit to be as large as possible, since this controls the
    # granularity with which we can modulate the PWM signal.  The following
    # settings allow us to set the fan from 0 to 100% in increments of 1/96th.
    #
    # The AST chip supports 3 different PWM clock configurations, but we only use
    # type M for now.
    echo 0 > $PWM_DIR/pwm_type_m_division_h
    echo 5 > $PWM_DIR/pwm_type_m_division_l
    echo 95 > $PWM_DIR/pwm_type_m_unit
    
    # On fby2, there are 2 fans connected.
    # Each fan uses same PWM input and provide one tacho output.
    # Here is the mapping between the fan and PWN/Tacho,
    # staring from the one from the edge
    # Fan 0: PWM 0, Tacho0
    # Fan 1: PWM 0, Tacho1
    #
    # On fby2.50, there are 4 fan connected.
    # Fan 0: PWM 0, Tacho0
    # Fan 1: PWM 0, Tacho2
    # Fan 2: PWM 0, Tacho1
    # Fan 3: PWM 0, Tacho3
    
    fan_num=0
    board_id=`cat /sys/class/gpio/gpio195/value`
    rev_id2=`cat /sys/class/gpio/gpio194/value`
    if [[ $board_id == "1" && $rev_id2 == "1" ]]; then
       spb_type=1
    else
       spb_type=0
    fi
    fan_type=`cat /sys/class/gpio/gpio54/value`
else
    spb_type=$(get_spb_type)
    fan_type=$(gpio_get DUAL_FAN_DETECT G6)
fi

if [[ $spb_type == "1" && $fan_type == "0" ]]; then
    # 0 base
    fan_num=3
    echo "FAN CONFIG : Dual Rotor FAN"
else
    # 0 base
    fan_num=1
    echo "FAN CONFIG : Single Rotor FAN"
fi

if [ $kernel_ver == 4 ]; then
    # For each fan, setting the type, and 100% initially
    for pwm in 0 1; do
        echo 0 > $PWM_DIR/pwm${pwm}_type
        echo 0 > $PWM_DIR/pwm${pwm}_rising
        echo 0 > $PWM_DIR/pwm${pwm}_falling
        echo 1 > $PWM_DIR/pwm${pwm}_en
    done
    
    # Enable Tach
    if [[ $spb_type == "1" && $fan_type == "1" ]]; then
        echo 0 > $PWM_DIR/tacho2_source
        echo 0 > $PWM_DIR/tacho3_source
        echo 1 > $PWM_DIR/tacho2_en
        echo 1 > $PWM_DIR/tacho3_en
    else 
        for tach in 0 $fan_num; do
            echo 0 > $PWM_DIR/tacho${tach}_source
        done
        t=0
        while [ $t -le $fan_num ]; do
            echo 1 > $PWM_DIR/tacho${t}_en
            t=$((t+1))
        done
    fi
fi
