#!/bin/sh

echo 70 > /sys/devices/platform/ast_pwm_tacho.0/pwm0_falling
echo 70 > /sys/devices/platform/ast_pwm_tacho.0/pwm1_falling
sleep 1
fan0_rpm=`cat /sys/devices/platform/ast_pwm_tacho.0/tacho0_rpm`
fan1_rpm=`cat /sys/devices/platform/ast_pwm_tacho.0/tacho1_rpm`

#restore the initial value 100% (init_pwm.sh)
echo 0 > /sys/devices/platform/ast_pwm_tacho.0/pwm0_falling
echo 0 > /sys/devices/platform/ast_pwm_tacho.0/pwm1_falling

if [ $fan0_rpm -ge "8000" ] || [ $fan1_rpm -ge "8000" ]; then
    echo 1 > /tmp/fan_config
else
    echo 0 > /tmp/fan_config
fi