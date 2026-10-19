#!/bin/sh
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
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
#shellcheck disable=SC1091
#shellcheck disable=SC2034

. /usr/local/bin/i2c-utils.sh
SCMCPLD_SYSFS_DIR=$(i2c_device_sysfs_abspath 1-0035)
HOST_POST_COMPLETE="${SCMCPLD_SYSFS_DIR}/bios_post_cmplt_l"

CHASSIS_SERVICE="xyz.openbmc_project.State.Chassis"
CHASSIS_INTERFACE="xyz.openbmc_project.State.Chassis"
CHASSIS_PROPERTY="CurrentPowerState"
CHASSIS_PROPERTY_STR="xyz.openbmc_project.State.Chassis"

HOST_SERVICE="xyz.openbmc_project.State.Host"
HOST_INTERFACE="xyz.openbmc_project.State.Host"
HOST_PROPERTY="CurrentHostState"
HOST_PROPERTY_STR="xyz.openbmc_project.State.Host"

BOOTPROGRESS_INTERFACE="xyz.openbmc_project.State.Boot.Progress"
BOOTPROGRESS_PROPERTY="BootProgress"
BOOTPROGRESS_PROPERTY_STR_BOOTING="xyz.openbmc_project.State.Boot.Progress.ProgressStages.Unspecified"
BOOTPROGRESS_PROPERTY_STR_BOOTCMPLT="xyz.openbmc_project.State.Boot.Progress.ProgressStages.OSStart"

chassis_object=$(busctl tree "$CHASSIS_SERVICE" --list | grep chassis | head -n 1)
host_object=$(busctl tree "$HOST_SERVICE" --list | grep host | head -n 1)

last_chassis_state=""
last_host_state=""
last_boot_progress_state=""

while true; do
    ret_val="$(/usr/local/bin/wedge_power.sh status)"
    if echo "$ret_val" | grep -q "on"; then
        curr_chassis_state="On"
        curr_host_state="Running"
    else
        curr_chassis_state="Off"
        curr_host_state="Off"
    fi

    curr_boot_progress_state=""
    if [ -f "$HOST_POST_COMPLETE" ]; then
        read -r post_val < "$HOST_POST_COMPLETE"
        if [ "$post_val" = "0x0" ]; then
            curr_boot_progress_state="$BOOTPROGRESS_PROPERTY_STR_BOOTCMPLT"
        else
            curr_boot_progress_state="$BOOTPROGRESS_PROPERTY_STR_BOOTING"
        fi
    fi

    # Quick diff comparison: If the CPLD hardware state hasn't changed, simply sleep and proceed to the next iteration
    if [ "$curr_chassis_state" = "$last_chassis_state" ] && \
       [ "$curr_host_state" = "$last_host_state" ] && \
       [ "$curr_boot_progress_state" = "$last_boot_progress_state" ]; then
        sleep 2
        continue
    fi

    # Only update DBus properties when the current state differs from the last known state, to avoid unnecessary DBus traffic.
    if [ ! "$curr_chassis_state" = "$last_chassis_state" ]; then
        pre_chassis_state=$(busctl get-property "$CHASSIS_SERVICE" "$chassis_object" "$CHASSIS_INTERFACE" "$CHASSIS_PROPERTY" | cut -d'"' -f2)
        target_chassis="${CHASSIS_PROPERTY_STR}.PowerState.${curr_chassis_state}"
        if [ ! "$target_chassis" = "$pre_chassis_state" ]; then
            echo "Chassis power state change detected, updating state to $curr_chassis_state"
            busctl set-property "$CHASSIS_SERVICE" "$chassis_object" "$CHASSIS_INTERFACE" \
                "$CHASSIS_PROPERTY" s "$target_chassis"
        fi
        last_chassis_state="$curr_chassis_state"
    fi
    # Set host state if mismatch detected
    if [ ! "$curr_host_state" = "$last_host_state" ]; then
        pre_host_state=$(busctl get-property "$HOST_SERVICE" "$host_object" "$HOST_INTERFACE" "$HOST_PROPERTY" | cut -d'"' -f2)
        target_host="${HOST_PROPERTY_STR}.HostState.${curr_host_state}"
        if [ ! "$target_host" = "$pre_host_state" ]; then
            echo "Host power state change detected, updating state to $curr_host_state"
            busctl set-property "$HOST_SERVICE" "$host_object" "$HOST_INTERFACE" \
                "$HOST_PROPERTY" s "$target_host"
        fi
        last_host_state="$curr_host_state"
    fi
    # Set boot progress if mismatch detected
    if [ ! "$curr_boot_progress_state" = "$last_boot_progress_state" ]; then
        pre_host_boot_progress_state=$(busctl get-property "$HOST_SERVICE" "$host_object" "$BOOTPROGRESS_INTERFACE" "$BOOTPROGRESS_PROPERTY" | cut -d'"' -f2)
        if [ ! "$curr_boot_progress_state" = "$pre_host_boot_progress_state" ]; then
            echo "Boot progress change detected, updating state to $curr_boot_progress_state"
            busctl set-property "$HOST_SERVICE" "$host_object" "$BOOTPROGRESS_INTERFACE" \
                "$BOOTPROGRESS_PROPERTY" s "$curr_boot_progress_state"
        fi
        last_boot_progress_state="$curr_boot_progress_state"
    fi

    sleep 1
done