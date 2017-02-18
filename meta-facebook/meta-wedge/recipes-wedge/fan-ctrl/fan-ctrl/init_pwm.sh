#!/bin/sh
#
# Copyright 2004-present Facebook. All Rights Reserved.
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
. /usr/local/bin/openbmc-utils.sh

PWM_DIR=/sys/devices/platform/ast_pwm_tacho.0

set -e

# Enable PWM 0, 1, 6, 7
devmem_set_bit $(scu_addr 88) 0
devmem_set_bit $(scu_addr 88) 1
devmem_set_bit $(scu_addr 88) 6
devmem_set_bit $(scu_addr 88) 7

# Eable tacho 0-7
devmem_clear_bit $(scu_addr 88) 8
devmem_clear_bit $(scu_addr 88) 9
devmem_clear_bit $(scu_addr 88) 10
devmem_clear_bit $(scu_addr 88) 11
devmem_clear_bit $(scu_addr 88) 12
devmem_clear_bit $(scu_addr 88) 13
devmem_clear_bit $(scu_addr 88) 14
devmem_clear_bit $(scu_addr 88) 15

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

# On wedge, there are 4 fans connected.
# Each fan has one PWM input and 2 tacho outputs.
# Here is the mapping between the fan and PWN/Tacho,
# staring from the one from the edge
# Fan 0: PWM 7, Tacho3, Tacho7
# Fan 1: PWM 6, Tacho2, Tacho6
# Fan 2: PWM 0, Tacho0, Tacho4
# Fan 3: PWM 1, Tacho1, Tacho5

# For each fan, setting the type, and 100% initially
for pwm in 0 1 6 7; do
    echo 0 > $PWM_DIR/pwm${pwm}_type
    echo 0 > $PWM_DIR/pwm${pwm}_rising
    echo 0 > $PWM_DIR/pwm${pwm}_falling
    echo 1 > $PWM_DIR/pwm${pwm}_en
done

# Enable Tach 0..7
echo 0 > $PWM_DIR/tacho0_source
echo 0 > $PWM_DIR/tacho4_source
echo 1 > $PWM_DIR/tacho1_source
echo 1 > $PWM_DIR/tacho5_source
echo 6 > $PWM_DIR/tacho2_source
echo 6 > $PWM_DIR/tacho6_source
echo 7 > $PWM_DIR/tacho3_source
echo 7 > $PWM_DIR/tacho7_source
t=0
while [ $t -le 7 ]; do
    echo 1 > $PWM_DIR/tacho${t}_en
    t=$((t+1))
done
