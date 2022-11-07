#!/bin/bash

# Copyright 2022-present Facebook. All Rights Reserved.

#
# Protect against multiple power requests
#
BOARD_UTILS_TIMEOUT=20
# Worst case in FPGA design is 10 seconds for power to drain before power on
# again. Add some buffer for time variation, errors, and serializing
# simultaneous requests.
if [[ "${FLOCKER}" != "$0" ]]; then
    exec env FLOCKER="$0" flock -w "$BOARD_UTILS_TIMEOUT" "$0" "$0" "$@"
fi

wedge_board_type() {
    echo 'SANDIA'
}

SEQ_STR="sequenced"
ON_STR="powered on"

wait_for_sequencing() {
    PSEQ=/sys/bus/platform/devices/pseq
    TIMEOUT=12
    COUNT=0
    if [ -e ${PSEQ} ]; then
        while grep -q "${SEQ_STR}" ${PSEQ}/power_state;
        do
            if [ ${COUNT} -ge ${TIMEOUT} ]; then
                #Error: Time out
                return 1
            fi
            sleep 1
            COUNT=$(( COUNT + 1 ))
        done
    fi

    return 0
}

userver_power_is_on() {
    PSEQ=/sys/bus/platform/devices/pseq
    if [ -e ${PSEQ} ]; then
        if grep -q "${ON_STR}" ${PSEQ}/power_state; then
            return 0 #powered on
        fi
    else
        i2cset -f -y 2 0x21 0x90
        i2cset -f -y 2 0x22 0x40
        i2cset -f -y 2 0x23 0x01
        i2cset -f -y 2 0x24 0x00
        i2cset -f -y 2 0x20 0x01
        power_state=$(i2cget -f -y 2 0x26)
        power_state=$((power_state >> 1))
        power_state=$((power_state & 0x03))
        if [ $power_state -eq 2 ]; then
            return 0 #powered on
        fi
    fi

    return 1 # not powered on
}

userver_power_on() {
    if ! wait_for_sequencing; then
        return 1
    fi

    # Safe to perform power operation
    MSD=/sys/bus/platform/devices/msd
    for ((i=0; i<20; i++)); do
        if [ -w ${MSD} ]; then
            echo 0 > ${MSD}/cfg7
            echo power-on > ${MSD}/control
            break
        else
            # Write 0x00000000 at Cyclonus 0x0000103c
            if ! i2cset -f -y 2 0x21 0x3c &> /dev/null; then
                sleep 10
                continue
            fi
            i2cset -f -y 2 0x22 0x10
            i2cset -f -y 2 0x23 0x00
            i2cset -f -y 2 0x24 0x00
            i2cset -f -y 2 0x25 0x00
            i2cset -f -y 2 0x26 0x00
            i2cset -f -y 2 0x27 0x00
            i2cset -f -y 2 0x28 0x00
            i2cset -f -y 2 0x20 0x00

	    # Trigger rising edge of bit 5 at Cyclonus 0x0000103c
            i2cset -f -y 2 0x25 0x20
            i2cset -f -y 2 0x20 0x00
            break
        fi
    done

    if wait_for_sequencing; then
        return 0
    else
        return 1
    fi
}

userver_power_off() {
    if ! wait_for_sequencing; then
        return 1
    fi

    # Safe to perform power operation
    MSD=/sys/bus/platform/devices/msd
    XIL=/sys/bus/platform/devices/xil
    if [ -w ${MSD} ]; then
        echo 0 > ${MSD}/cfg7
        echo power-off > ${MSD}/control
        if [ -w ${XIL} ]; then
            # Reset SMB power
            echo 0 > ${XIL}/cfg7
            echo 0x400 > ${XIL}/cfg7
        else
            #Error: XIL driver not loaded
            return 1
        fi
    else
        # Write 0x00000000 at Cyclonus 0x0000103c
        i2cset -f -y 2 0x21 0x3c
        i2cset -f -y 2 0x22 0x10
        i2cset -f -y 2 0x23 0x00
        i2cset -f -y 2 0x24 0x00
        i2cset -f -y 2 0x25 0x00
        i2cset -f -y 2 0x26 0x00
        i2cset -f -y 2 0x27 0x00
        i2cset -f -y 2 0x28 0x00
        i2cset -f -y 2 0x20 0x00

        # Trigger rising edge of bit 3 at Cyclonus 0x0000103c
        i2cset -f -y 2 0x25 0x08
        i2cset -f -y 2 0x20 0x00
    fi

    if wait_for_sequencing; then
        return 0
    else
        return 1
    fi
}

userver_reset() {
    if ! wait_for_sequencing; then
        return 1
    fi

    MSD=/sys/bus/platform/devices/msd
    XIL=/sys/bus/platform/devices/xil
    if [ -w ${MSD} ]; then
        echo 0 > ${MSD}/cfg7
        echo cold-reset > ${MSD}/control
        if [ -w ${XIL} ]; then
            # Reset SMB power
            echo 0 > ${XIL}/cfg7
            echo 0x400 > ${XIL}/cfg7
        else
            #Error: XIL driver not loaded
            return 1
        fi
    else
        # Write 0x00000000 at Cyclonus 0x0000103c
        i2cset -f -y 2 0x21 0x3c
        i2cset -f -y 2 0x22 0x10
        i2cset -f -y 2 0x23 0x00
        i2cset -f -y 2 0x24 0x00
        i2cset -f -y 2 0x25 0x00
        i2cset -f -y 2 0x26 0x00
        i2cset -f -y 2 0x27 0x00
        i2cset -f -y 2 0x28 0x00
        i2cset -f -y 2 0x20 0x00

        # Trigger rising edge of bit 6 at Cyclonus 0x0000103c
        i2cset -f -y 2 0x25 0x40
        i2cset -f -y 2 0x20 0x00
    fi

    if wait_for_sequencing; then
        return 0
    else
        return 1
    fi
}

chassis_power_cycle() {
    if ! wait_for_sequencing; then
        return 1
    fi

    MSD=/sys/bus/platform/devices/msd
    if [ -w ${MSD} ]; then
        echo 0 > ${MSD}/cfg7
        echo 0x400 > ${MSD}/cfg7
    else
        # Write 0x00000000 at Cyclonus 0x0000103c
        i2cset -f -y 2 0x21 0x3c
        i2cset -f -y 2 0x22 0x10
        i2cset -f -y 2 0x23 0x00
        i2cset -f -y 2 0x24 0x00
        i2cset -f -y 2 0x25 0x00
        i2cset -f -y 2 0x26 0x00
        i2cset -f -y 2 0x27 0x00
        i2cset -f -y 2 0x28 0x00
        i2cset -f -y 2 0x20 0x00

        # Trigger rising edge of bit 10 at Cyclonus 0x0000103c
        i2cset -f -y 2 0x26 0x04
        i2cset -f -y 2 0x20 0x00
    fi

    # Power sequncers take 10 seconds to complete.
    if wait_for_sequencing; then
        return 0
    else
        return 1
    fi
}

bmc_mac_addr() {
    mac=00:00:00:00:00:00

    hexdump -ve '1/1 "%.2x"' "$SYSFS_I2C_DEVICES"/8-0054/eeprom > /tmp/eeprom_data

    ((i = 0))
    while [[ $i -lt 512 ]]; do
        var=$( dd skip=$i count=4 if=/tmp/eeprom_data bs=1 2> /dev/null )
        if [ "$var" = "cf06" ];then
            ((i = i + 4))
            mac1=$( dd skip=$i count=2 if=/tmp/eeprom_data bs=1 2> /dev/null)
            ((i = i + 2))
            mac2=$( dd skip=$i count=2 if=/tmp/eeprom_data bs=1 2> /dev/null)
            ((i = i + 2))
            mac3=$( dd skip=$i count=2 if=/tmp/eeprom_data bs=1 2> /dev/null)
            ((i = i + 2))
            mac4=$( dd skip=$i count=2 if=/tmp/eeprom_data bs=1 2> /dev/null)
            ((i = i + 2))
            mac5=$( dd skip=$i count=2 if=/tmp/eeprom_data bs=1 2> /dev/null)
            ((i = i + 2))
            mac6=$( dd skip=$i count=2 if=/tmp/eeprom_data bs=1 2> /dev/null)

            mac=$mac1:$mac2:$mac3:$mac4:$mac5:$mac6
            break
        fi
        ((i = i + 1))
    done

    rm /tmp/eeprom_data
    echo "$mac"
}

userver_mac_addr() {
    weutil --json | jq -r '."Extended MAC Base"'
}
