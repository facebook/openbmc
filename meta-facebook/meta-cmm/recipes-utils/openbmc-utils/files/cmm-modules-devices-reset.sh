#!/bin/bash
#
# Copyright 2019-present Facebook. All Rights Reserved.
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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

declare -a DEVICES
DEVICES=(
  I2C:0x1
  SCM-BMC:0x2  
)

declare -a MODULES
MODULES=(
  LC101:0x40
  LC102:0x50
  LC201:0x41
  LC202:0x51
  LC301:0x42
  LC302:0x52
  LC401:0x43
  LC402:0x53
  FC1:0x88
  FC2:0x89
  FC3:0x8a
  FC4:0x8b
  SCM-FC1-LEFT:0xc2
  SCM-FC1-RIGHT:0xd2
  SCM-FC2-LEFT:0xc3
  SCM-FC2-RIGHT:0xd3
  SCM-LC101:0xc4
  SCM-LC102:0xd4
  SCM-LC201:0xc5
  SCM-LC202:0xd5
  SCM-LC301:0xc6
  SCM-LC302:0xd6
  SCM-LC401:0xc7
  SCM-LC402:0xd7
  SCM-FC3-LEFT:0xc8
  SCM-FC3-RIGHT:0xd8
  SCM-FC4-LEFT:0xc9
  SCM-FC4-RIGHT:0xd9
)

#Resetting all the devices of a module
resetModuleDeviceAll(){
    for key in ${MODULES[*]}; do
        module=${key%%:*}
        m_value=${key##*:}
        if [ "${module}" = $1 ]; then
            echo "Resetting ${module} module and the device on SMC-BMC and the i2c endpoints"
            for dev in ${DEVICES[*]}; do
                device=${dev%%:*}
                d_value=${dev##*:}
                echo "Resetting ${device} devices of ${module} module."
                sleep 1
                echo ${d_value} > /sys/bus/i2c/drivers/cmmcpld/13-003e/one_wire_rst_device
                echo ${m_value} > /sys/bus/i2c/drivers/cmmcpld/13-003e/one_wire_rst_trig
            done
            break
        fi
    done
    if [ ${module} != $1 ]; then
        echo "Unsupported Module entered or wrong command usage. To see list of supported modules/devices"
        echo "Please run 'cmm-modules-devices-reset view-modules-devices' without the single quotes."
        echo "Command Usage is: <cmm-modules-devices-reset.sh><module_name>[Optional:which_device] or "
        echo "<cmm-modules-devices-reset.sh><module_name><all>"
    fi
  unset module value
}

#Resetting a specific device from a module
resetModuleDevice(){
    for key in ${MODULES[*]}; do
        module=${key%%:*}
        m_value=${key##*:}
        if [ "${module}" = $1 ]; then
            if [ $2 = "all" ]; then
                resetModuleDeviceAll $module
                unset module m_value
                return
            else
                for dev in ${DEVICES[*]}; do
                    device=${dev%%:*}
                    d_value=${dev##*:}
                    if [ "${device}" = $2 ]; then
                        echo "Resetting ${device} devices of ${module} module."
                        sleep 1
                        echo ${d_value} > /sys/bus/i2c/drivers/cmmcpld/13-003e/one_wire_rst_device
                        echo ${m_value} > /sys/bus/i2c/drivers/cmmcpld/13-003e/one_wire_rst_trig
                        unset module m_value device d_value
                        return
                    fi
                done
            fi
        fi
    done
    if  [ ${module} != $1 ] || [ ${device} != $2 ]; then 
        echo "Unsupported Module/Device entered or wrong command usage. To see list of supported modules/devices"
        echo "Please run 'cmm-modules-devices-reset view-modules-devices' without the single quotes."
        echo "Command Usage is: <cmm-modules-devices-reset.sh><module_name>[Optional:which_device] or "
        echo "<cmm-modules-devices-reset.sh><module_name><all>"
    fi
    unset module m_value device d_value
}

#List supported modules and devices for the user
viewModuleDevices(){
    echo "Listing available modules:"
    for key in ${MODULES[*]}; do
        module=${key%%:*}
        echo "$module"     
    done
    echo -e "\nListing available devices:"

    for key in ${DEVICES[*]}; do
        device=${key%%:*}
        echo "$device"     
    done
    unset module device
}

#parse command line arguments
if [ $# == 0 ]; then
      echo "please enter the module name."
      echo "If unsure what modules/devices are available"
      echo "Please run 'cmm-modules-devices-reset view-modules-devices' without the single quotes."
elif [ $# == 1 ]; then 
    if [ $1 = "view-modules-devices" ]; then
          viewModuleDevices
      else
          resetModuleDeviceAll $1
     fi
elif [ $# == 2 ]; then
      resetModuleDevice $1 $2
else
      echo "error wrong command"
      exit 1
fi
