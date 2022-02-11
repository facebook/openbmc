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
PWM_DIR=/sys/devices/platform/ast_pwm_tacho.0

set -e

# The PWM frequency is

#   clk_source / ((2 ^ division_h) * (2 * division_l) * (unit + 1))
#
# Our clk_source is 24Mhz.  4-pin fans are generally supposed to be driven with
# a 25Khz PWM control signal.  Therefore we want the divisor to equal 800.
#
# We also want the unit to be as large as possible, since this controls the
# granularity with which we can modulate the PWM signal.  The following
# settings allow us to set the fan from 0 to 100% in increments of 1/100th.
#
# The AST chip supports 3 different PWM clock configurations, but we only use
# type M for now.
echo 0 > $PWM_DIR/pwm_type_m_division_h
echo 4 > $PWM_DIR/pwm_type_m_division_l
echo 99 > $PWM_DIR/pwm_type_m_unit
# Set Type M tacho clock division to 4
echo 0 > $PWM_DIR/tacho_type_m_division

# On FBTTN, there are 4 fans (dual-rotate) connected on DPB.
# Each fan uses same PWM input.
# Tacho output is used for BMC or SCC heartbeat.
# Here is the mapping between the fan and PWN/Tacho,
# staring from the one from the edge
# Fan 0: PWM 0
# Fan 1: PWM 0
# BMC Remote Heartbeat: PWM 0, Type M, Tacho0
# SCC Local Heartbeat:  PWM 0, Type M, Tacho4
# SCC Remote Heartbeat: PWM 0, Type M, Tacho5

# For each fan, setting the type, and 100% initially
# PWM0, PWM1: Type M
for pwm in 0 1; do
    echo 0 > $PWM_DIR/pwm${pwm}_type
    echo 0 > $PWM_DIR/pwm${pwm}_rising
    echo 0 > $PWM_DIR/pwm${pwm}_falling
    echo 1 > $PWM_DIR/pwm${pwm}_en
done

# Enable Tach 0, 4, 5
for tach in 0 4 5; do
    echo 0 > $PWM_DIR/tacho${tach}_source
    echo 1 > $PWM_DIR/tacho${tach}_en
done
