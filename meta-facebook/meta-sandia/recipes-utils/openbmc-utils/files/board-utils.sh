#!/bin/bash

# Copyright 2022-present Facebook. All Rights Reserved.

wedge_board_type() {
    echo 'SANDIA'
}

userver_power_is_on() {
    echo "FIXME: userver_power_is_on() not implemented!"
    return 1
}

userver_power_on() {
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

    # Trigger rising edge of bit 5 at Cyclonus 0x0000103c
    i2cset -f -y 2 0x25 0x20
    i2cset -f -y 2 0x20 0x00
}

userver_power_off() {
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
}

userver_reset() {
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
}

chassis_power_cycle() {
    echo "FIXME: chassis_power_cycle() not implemented!"
    return 1
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
