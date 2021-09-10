#!/bin/bash

# Throw everything to stdout so that svlogd can capture it
exec 2>&1

set -x
set -e
set -u
set -o pipefail
export PATH=/usr/local/bin:/usr/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin:.

spi_util_cmd="/usr/local/bin/spi_util.sh"


function open_isolation_buffer() {
    echo 0x1 > /sys/bus/i2c/devices/12-0031/com-e_i2c_isobuf_en
}

function change_gpios() {
    # These paths in /tmp are human-readable symlinks to GPIO numbers
    # in /sys/class/gpìo
    echo out > /tmp/gpionames/SWITCH_EEPROM1_WRT/direction
    echo 1 > /tmp/gpionames/SWITCH_EEPROM1_WRT/value
}

eeprom_io_legacy() {
    io_type="$1"
    image_file="$2"

    at93cx6_util_py3.py chip "$io_type" --file "$image_file"
}

eeprom_io_new() {
    io_type="$1"
    image_file="$2"

    "$spi_util_cmd" "$io_type" spi2 OOB_SWITCH_EEPROM "$image_file"
}

oob_switch_eeprom_program() {
    input_image="$1"
    tmp_file="/tmp/.oob_eeprom.tmp"

    if grep OOB_SWITCH_EEPROM "$spi_util_cmd"; then
        eeprom_io_new write "$input_image"
        eeprom_io_new read "$tmp_file"
    else
        change_gpios
        eeprom_io_legacy write "$input_image"
        eeprom_io_legacy read "$tmp_file"
    fi

    diff "$input_image" "$tmp_file"
    rm -f "$tmp_file"
}

open_isolation_buffer

oob_switch_eeprom_program $1

