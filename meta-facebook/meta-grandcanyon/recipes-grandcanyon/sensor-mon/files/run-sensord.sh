#!/bin/sh

# shellcheck source=/usr/local/fbpackages/utils/ast-functions
. /usr/local/fbpackages/utils/ast-functions

echo "Setup sensor monitoring for FBGC... "

ret=0

FRUS="server uic nic"
if [ "$(is_e1s_iocm_i2c_enabled)" -eq "1" ]; then
  FRUS="$FRUS e1s_iocm"
fi

if [ "$(is_chassis_type7)" -eq "1" ]; then
  if [ "$(is_e1s_iocm_i2c_enabled)" -eq "1" ]; then
    # reload drivers
    if [ -d "/sys/class/i2c-dev/i2c-13/device/13-004a" ]; then
      echo "0x4a" > /sys/class/i2c-dev/i2c-13/device/delete_device
    fi
    echo "tmp75 0x4a" > /sys/class/i2c-dev/i2c-13/device/new_device
    ret=$?
    if [ "$ret" -ne "0" ]; then
      logger -t "sensord" "echo tmp75 0x4a to i2c-13 new_device failed: ret: $((ret))"
    fi
    if [ "$(is_module_loaded ti_ads1015)" -eq "1" ]; then
      rmmod ti_ads1015
      ret=$?
      if [ "$ret" -ne "0" ]; then
        logger -t "sensord" "rmmod ti_ads1015 failed ret: $((ret))"
      fi
    fi
    modprobe ti_ads1015
  else
    #unload drivers
    if [ "$(is_module_loaded ti_ads1015)" -eq "1" ]; then
      rmmod ti_ads1015
      ret=$?
      if [ "$ret" -ne "0" ]; then
        logger -t "sensord" "rmmod ti_ads1015 failed ret: $((ret))"
      fi
    fi
    if [ -d "/sys/class/i2c-dev/i2c-13/device/13-004a" ]; then
      echo "0x4a" > /sys/class/i2c-dev/i2c-13/device/delete_device
    fi
  fi
fi

exec /usr/local/bin/sensord $FRUS
