#!/bin/bash

# Copyright 2022-present Facebook. All Rights Reserved.

# shellcheck disable=SC2034

wedge_board_type() {
    echo 'FBDARWIN'
}

wedge_slot_id() {
    printf "1\n"
}

wedge_board_rev() {
    # Assume P1
    return 1
}

retry_command() {
    # Retry command up to $1 attempts
    local retries=$1
    shift

    local count=0
    until "$@"; do
        exit=$?
        count=$((count+1))
        if [ "$count" -lt "$retries" ]; then
            msg="Attempt $count/$retries failed with $exit, retrying..."
            echo "$msg"
            logger retry_command: "$msg"
        else
            msg="Retry $count/$retries failed with $exit, no more retries left"
            echo "$msg"
            logger retry_command: "$msg"
            return $exit
        fi
    done
    return 0
}

do_sync() {
   # Try to flush all data to MMC and remount the drive read-only. Sync again
   # because the remount is asynchronous.
   partition="/dev/mmcblk0"
   mountpoint="/mnt/data1"

   sync
   # rsyslogd logs to eMMC, so we must stop it prior to remouting.
   systemctl stop syslog.socket rsyslog.service
   sleep .05
   if [ -e $mountpoint ]; then
      retry_command 5 mount -o remount,ro $partition $mountpoint
      sleep 1
   fi
   sync
}

userver_power_is_on() {
    return 0
}

userver_power_on() {
    return 0
}

userver_power_off() {
    echo "FBDARWIN doesn't support SCM power off!"
    return 1
}

userver_reset() {
    retries=3
    for _ in $(seq $retries); do
        gpio_set_value BMC_SYS_PWR_CYC1 1
        sleep 1
        gpio_set_value BMC_SYS_PWR_CYC1 0
        sleep 5
    done
}

chassis_power_cycle() {
    do_sync

    gpio_set_value BMC_SYS_PWR_CYC0 1
    sleep 8

    # The chassis shall be reset now... if not, we are in trouble
    echo "ERROR: unable to power cycle chassis!!!"
    gpio_set_value BMC_SYS_PWR_CYC0 0
    exit 1
}

bmc_mac_addr() {
    weutil bmc | grep '^Extended MAC Base' | cut -d' ' -f 4
}

userver_mac_addr() {
    weutil CHASSIS 2>&1 | grep "MAC Base" | cut -d ' ' -f 4
}
