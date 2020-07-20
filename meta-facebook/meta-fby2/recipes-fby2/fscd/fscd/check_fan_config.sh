#!/bin/sh

. /usr/local/fbpackages/utils/ast-functions

spb_type=$(get_spb_type)
kernel_ver=$(get_kernel_ver)


if [ $kernel_ver == 5 ]; then
    set_fan1='echo 180 > /sys/devices/platform/ahb/ahb:apb/1e786000.pwm-tacho-controller/hwmon/hwmon*/pwm1'
    set_fan2='echo 180 > /sys/devices/platform/ahb/ahb:apb/1e786000.pwm-tacho-controller/hwmon/hwmon*/pwm2'
    eval $set_fan1
    eval $set_fan2
    sleep 1
    if [ $spb_type == 1 ] ; then
        # for Yv2.50
        fan0_rpm=`cat /sys/bus/platform/drivers/aspeed_pwm_tacho/1e786000.pwm-tacho-controller/hwmon/hwmon*/fan3_input`
        fan1_rpm=`cat /sys/bus/platform/drivers/aspeed_pwm_tacho/1e786000.pwm-tacho-controller/hwmon/hwmon*/fan4_input`
    else
        fan0_rpm=`cat /sys/bus/platform/drivers/aspeed_pwm_tacho/1e786000.pwm-tacho-controller/hwmon/hwmon*/fan1_input`
        fan1_rpm=`cat /sys/bus/platform/drivers/aspeed_pwm_tacho/1e786000.pwm-tacho-controller/hwmon/hwmon*/fan2_input`
    fi
    #restore the initial value 100% (init_pwm.sh)
    reset_fan1='echo 0 > /sys/devices/platform/ahb/ahb:apb/1e786000.pwm-tacho-controller/hwmon/hwmon*/pwm1'
    reset_fan2='echo 0 > /sys/devices/platform/ahb/ahb:apb/1e786000.pwm-tacho-controller/hwmon/hwmon*/pwm2'
    eval $reset_fan1
    eval $reset_fan2
else
    echo 70 > /sys/devices/platform/ast_pwm_tacho.0/pwm0_falling
    echo 70 > /sys/devices/platform/ast_pwm_tacho.0/pwm1_falling
    sleep 1
    if [ $spb_type == 1 ] ; then
        # for Yv2.50
        fan0_rpm=`cat /sys/devices/platform/ast_pwm_tacho.0/tacho2_rpm`
        fan1_rpm=`cat /sys/devices/platform/ast_pwm_tacho.0/tacho3_rpm`
    else
        fan0_rpm=`cat /sys/devices/platform/ast_pwm_tacho.0/tacho0_rpm`
        fan1_rpm=`cat /sys/devices/platform/ast_pwm_tacho.0/tacho1_rpm`
    fi
    #restore the initial value 100% (init_pwm.sh)
    echo 0 > /sys/devices/platform/ast_pwm_tacho.0/pwm0_falling
    echo 0 > /sys/devices/platform/ast_pwm_tacho.0/pwm1_falling
fi

if [ $fan0_rpm -ge "8000" ] || [ $fan1_rpm -ge "8000" ]; then
    echo 1 > /tmp/fan_config
else
    echo 0 > /tmp/fan_config
fi