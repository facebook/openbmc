#!/bin/bash
#
# Copyright 2022-present Facebook. All Rights Reserved.
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
### BEGIN INIT INFO
# Provides:          find_serfmon.sh
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description:  Find Serfmon string from mTerm log and cache it
### END INIT INFO

# shellcheck disable=SC1091
. /usr/bin/bmc_eeprom_checker.sh

MTERM_LOGFILE="/var/log/mTerm_wedge.log"
SERFMON_CACHE="/mnt/data1/.serfmon.txt"
MACMON_CACHE="/mnt/data1/.macmon.txt"
CHASSIS_EEPROM_CACHE="/mnt/data1/.chassis_eeprom"
META_EEPROM_CACHE="/mnt/data1/.chassis_meta_eeprom"
SERFMON_REGEX="\!serfmon\:.*\:.*\:.*$"
MACMON_REGEX="\!macmon\:.*\:.*\:.*$"

crc16_ccitt() {
    # Calculate CRC16-CCITT for the given bytes as specified in
    # https://srecord.sourceforge.net/crc16-ccitt.html#overview
    local data_file=$1
    local crc_work_file=/tmp/.crc16_ccitt
    crc=0xffff

   cp "$data_file" "$crc_work_file"
   printf "\x00\x00" >> "$crc_work_file"

    while IFS= read -r -n 1 -d '' input_byte
    do
        printf -v byte_val "%d" \'"$input_byte"
        for (( i=7; i>=0; i-- )); do
            new_crc=$(( ((crc << 1) & 0xffff) | ((byte_val >> i)) & 0x1 ))
            if [ $((crc & 0x8000)) != 0 ]; then
                crc=$((new_crc ^ 0x1021))
            else
                crc=$new_crc
            fi
        done
    done < "$crc_work_file"

    # Enforce 4 digits in return value.
    rm -f "$crc_work_file"
    printf -v crc "%04x" $crc
    echo $crc
}

write_pad_bytes() {
    # Write num_bytes of all \xFF to the file tmpfile.
    local num_bytes=$1
    local tmpfile=$2
    dd if=/dev/zero bs=1 count="$num_bytes" \
       2>/dev/null | tr "\000" "\377" >> "$tmpfile"
}

write_raw_bytes() {
    # Given string in the format 00112233, write the bytes to the
    # given file as 8-bit characters \x00\x11\x22\x33
    local bytes=$1
    local tmpfile=$2
    for (( i=0; i<${#bytes}; i=i+2 )); do
       # shellcheck disable=SC2059
       printf "\\x$(printf "%x" 0x"${bytes:$i:2}")" >> "$tmpfile"
    done
}

create_dummy_chassis_eeprom() {
    dummy_pca="PCA000000000"

    # Don't create dummy EEPROM until serfmon string is seen
    if [ ! -f "$SERFMON_CACHE" ]; then
       return
    fi

    # Extract SN from Serfmon Format \!serfmon\:.*\:.*\:.*$
    chassis_serial="$(awk -F":" '{print $3}' "$SERFMON_CACHE")"

    TMP="${CHASSIS_EEPROM_CACHE}.tmp"

    # Create Fake EEPROM file for weutil in expected format
    printf "\x00\x00\x00\x03\x00\x00\x00\x00\x30\x30\x30\x32%s%s\x31\x31\x31" \
        "$dummy_pca" "$chassis_serial" > "$TMP"

    # How many bytes to pad EEPROM file
    pad_bytes=211

    # If a cached Macmon string exists, add that to the EEPROM
    if [ -f "$MACMON_CACHE" ]; then
       chassis_mac="$(awk -F":" '{print $3}' "$MACMON_CACHE")"
       # Add MAC TLV. MAC is 12 (0x43 = C) bytes in length
       printf "\x30\x35\x30\x30\x30\x43%s" "$chassis_mac" >> "$TMP"
       pad_bytes=$((pad_bytes - 18))
    fi

    # Add an end TLV.
    printf "\x30\x30\x30\x30\x30\x30" >> "$TMP"

    # Pad file with all 0xFFs to size 255.
    write_pad_bytes $pad_bytes "$TMP"

    # If the BMC EEPROM has been programmed with Meta EEPROM format, then assume
    # the chassis EEPROM should as well.
    if bmc_has_meta_eeprom; then
        META_FMT_TMP="${META_EEPROM_CACHE}.tmp"

        # Write the header and chassis fields.
        printf "\xfb\xfb\x05\xff\x01\x09%s\x06\x0c%s\x0b\x0b%s" \
            "DARWIN48V" "$dummy_pca" "$chassis_serial" > "$META_FMT_TMP"

        # Write a TLV for the CPU MAC address and a TLV for the Switch Asic MAC
        # address.
        if [ -f "$MACMON_CACHE" ]; then
            # size 8 = 6 bytes of MAC + 2 bytes for number of addresses.
            printf "\x11\x08" >> "$META_FMT_TMP"
            write_raw_bytes "$chassis_mac" "$META_FMT_TMP"
            printf "\x00\x01" >> "$META_FMT_TMP"

            # Switch ASIC MAC base = x86 CPU MAC base + 1.
            printf "\x13\x08" >> "$META_FMT_TMP"
            chassis_mac_dec=$(printf '%d\n' 0x"$chassis_mac")
            asic_mac=$(printf '%X' $((chassis_mac_dec + 1)))
            write_raw_bytes "$asic_mac" "$META_FMT_TMP"
            printf "\x01\x03" >> "$META_FMT_TMP"
        fi

        # Write the CRC TLV, then calculate the CRC value and write it.
        crc=$(crc16_ccitt "$META_FMT_TMP")
        printf "\xfa\x02" >> "$META_FMT_TMP"
        write_raw_bytes "$crc" "$META_FMT_TMP"

        # Pad bytes to Meta EEPROM, then write the Meta EEPROM data.
        write_pad_bytes $((META_EEPROM_OFFSET-255)) "$TMP"
        cat "$META_FMT_TMP" >> "$TMP"
    fi

    mv $TMP $CHASSIS_EEPROM_CACHE
}

maybe_update_cache() {
    # Check the mTerm logfile for the given regex. If found, update the cache_file
    local -n cached_value="$1"
    local -n cache_updated="$2"
    regex="$3"
    cache_file="$4"
    name="$5"

    # Always clear update flag first
    cache_updated=0

    if grep -qE "$regex" "$MTERM_LOGFILE"; then
        # Detect if string found in mTerm log
        read_value="$(grep -oE "$regex" "$MTERM_LOGFILE" | tail -n 1)"

        # Read cached string from eMMC
        if [ -f "$cache_file" ]; then
            cached_value="$(cat "$cache_file")"
        else
            cached_value=""
        fi

        # If mTerm's string value doesn't match the cache, update it
        if [ "$read_value" != "$cached_value" ]; then
            echo "$read_value" > "$cache_file"
            echo "Registered new $name string!"
            cached_value="$(cat "$cache_file")"
            echo "$read_value"
            # shellcheck disable=SC2034
            cache_updated=1
        fi
    fi
}

# Print cached Serfmon/Macmon strings on starting service
cached_serfmon=""
if [ -f "$SERFMON_CACHE" ]; then
    cached_serfmon="$(cat "$SERFMON_CACHE")"
    echo "$cached_serfmon"
fi

cached_macmon=""
if [ -f "$MACMON_CACHE" ]; then
    cached_macmon="$(cat "$MACMON_CACHE")"
    echo "$cached_macmon"
fi

if [ "$cached_serfmon" != "" ] || [ "$cached_macmon" != "" ]; then
    create_dummy_chassis_eeprom
fi

serfmon_updated=0
macmon_updated=0
while true; do
    if [ -f "$MTERM_LOGFILE" ]; then
        maybe_update_cache cached_serfmon serfmon_updated "$SERFMON_REGEX" \
            "$SERFMON_CACHE" "Serfmon"
        maybe_update_cache cached_macmon macmon_updated "$MACMON_REGEX" \
            "$MACMON_CACHE" "Macmon"

        if [ "$serfmon_updated" != 0 ] || [ "$macmon_updated" != 0 ]; then
            create_dummy_chassis_eeprom
        fi
    fi

    # Sleep 15 seconds until the next loop.
    # reduce for faster scan.
    sleep 15
done
