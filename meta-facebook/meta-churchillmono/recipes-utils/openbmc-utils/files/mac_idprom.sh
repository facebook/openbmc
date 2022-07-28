#!/bin/bash
#shellcheck disable=SC1091
#shellcheck disable=SC2001
#shellcheck disable=SC2059
. /usr/local/bin/openbmc-utils.sh
gpio_export_by_name "${ASPEED_GPIO}" GPIOG4 TXD8_SD2DAT2_SALT13 2> /dev/null

cached_eeprom_dir=/mnt/data1/mac_idprom
cache_eeprom_file=/mnt/data1/mac_idprom/cached_eeprom_54
eeprom_file=/sys/bus/i2c/devices/8-0054/eeprom
crc_cal_file=/usr/local/bin/crc_16.py
startfile=/tmp/startfile
endfile=/tmp/endfile
erasefile=/tmp/erasefile
inputfile=/tmp/inputfile
eeprom_data=/tmp/eeprom_data

# Start Bytes
printf '\xcf\x06' > $startfile
# End Bytes
printf '\x43\x00\x01' > $endfile
# ff bytes for paddding at the end
printf '\xff' > $erasefile

prog=$0

usage() {
    echo
    echo "Usage: $prog <Mac_Address>"
    echo
    echo "Mac Address:"
    echo "  Format(Hex): xx:xx:xx:xx:xx:xx "
    echo "  Example    : d4:eb:68:b5:65:68"
    echo
}

clean_up() {
    # cleaning up temp files
    rm $startfile 2> /dev/null
    rm $endfile 2> /dev/null
    rm $erasefile 2> /dev/null
    rm $inputfile 2> /dev/null
}

get_idprom_permission () {

    perm=$( gpio_get_value TXD8_SD2DAT2_SALT13 )
    echo "$perm"

}

set_idprom_permission () {

    gpio_set_value TXD8_SD2DAT2_SALT13 "$1"

}

padding_cache_eeprom_mac () {

    ((i = "${1}"))
    while [[ $i -lt 512 ]]
    do
        dd seek=$i count=1 if=$erasefile of=$cache_eeprom_file bs=1 2> /dev/null
        ((i = i +1))
    done

}

write_cache_eeprom_mac () {

    ((i = "${1}")) 
    dd seek=$i count=2 if=$startfile of=$cache_eeprom_file bs=1 2> /dev/null
    ((i = i + 2))
    dd seek=$i count=6 if=$inputfile of=$cache_eeprom_file bs=1 2> /dev/null
    end=$((i + 6))
    dd seek=$end count=3 if=$endfile of=$cache_eeprom_file bs=1 2> /dev/null
    end=$((end + 3))
    echo $end
}

read_eeprom_end() {

    hexdump -ve '1/1 "%.2x"' $cache_eeprom_file > $eeprom_data

    ((i = 0))
    while [[ $i -lt 512 ]]
    do
        # Looks for end of the eeprom content
        # So, it checks for ffffffff bytes to mark the end
        var=$( dd skip=$i count=8 if=$eeprom_data bs=1 2> /dev/null )
        if [ "$var" = "ffffffff" ];then
            echo $((i / 2))
            break
        fi
        ((i = i + 1))
    done

    rm $eeprom_data

}

check_crc_mac_address() {

    # Check for CRC of eeprom
    crc=$(python $crc_cal_file $eeprom_file check)
    if [[ $crc -eq 1 ]];then
        echo "CRC is Valid"
    else
        echo "Invalid CRC ... Fatal Error !"
        echo "FAIL"
    fi

    # Check for programmed mac address in eeprom
    mac_address=$(read_x86_mac_from_eeprom)

    if [[ $mac_address = "$input_mac_address" ]]; then
        echo "x86 mac address programmed successfully"
        echo "PASS"
    else
        echo "x86 mac address is not programmed"
        echo "FAIL"
    fi

}

if [ $# -lt 1 ]; then
    usage
    clean_up
    echo "FAIL"
    exit 1
fi

if ! [[ $1 =~ ^([[:xdigit:]]{2}:){5}[[:xdigit:]]{2}$ ]]; then
    echo "Invalid mac address"
    usage
    clean_up
    echo "FAIL"
    exit 1
fi

input_mac_address="$1"
arg_str=$(echo "$1" | sed 's/:/\\x/g')
input_str="\x$arg_str"
printf "$input_str" > $inputfile

# Check for mac address in eeprom
mac_address=$(read_x86_mac_from_eeprom)
mac_address_present=0

if [[ $mac_address =~ ^([[:xdigit:]]{2}:){5}[[:xdigit:]]{2}$ ]]; then
    echo "x86 mac address is present"
    mac_address_present=1
else
    echo "No x86 mac address"
fi

# CRC check
crc=$(python $crc_cal_file $eeprom_file check)

if [[ $crc -eq 1 ]];then
    echo "CRC is Valid"
    if [[ $mac_address_present -eq 1 ]];then
        clean_up
        echo "PASS"
        exit 1
    fi
    mkdir -p $cached_eeprom_dir
    # Copying EEPROM to buffer
    dd if=$eeprom_file of=$cache_eeprom_file 2> /dev/null
    sync
else
    echo "Invalid CRC... checking for cached eeprom"
    if [  -f  "$cache_eeprom_file" ]; then
        echo "Cache file is present"
        crc=$(python $crc_cal_file $cache_eeprom_file check)
        if [[ $crc -eq 1 ]];then
            set_idprom_permission 0
            dd if=$cache_eeprom_file of=$eeprom_file 2> /dev/null
            set_idprom_permission 1
            check_crc_mac_address
            clean_up
            exit 1
        else
            echo "Fatal Error! Cached eeprom has bad CRC. Unable to repair IDPROM"
            clean_up
            echo "FAIL"
            exit 1
        fi
    else
        echo "Fatal Error! IDPROM is corrupt and no cached IDPROM is available. Unable to repair IDPROM"
        clean_up
        echo "FAIL"
        exit 1
    fi
fi

# Get start address to write from
start_address=$(read_eeprom_end)

# Writing x86 mac address to eeprom
end_address=$(write_cache_eeprom_mac "$start_address")

# Padding ff to end of the buffer
padding_cache_eeprom_mac "$end_address"

# Updating the CRC for eeprom 
# after mac address addition
python $crc_cal_file $cache_eeprom_file

# Setting write permission
# 0 - permission to write
# 1 - permission denial
set_idprom_permission 0

# Copying buffer to EEPROM
dd if=$cache_eeprom_file of=$eeprom_file 2> /dev/null

# Setting back write protection
set_idprom_permission 1

check_crc_mac_address

clean_up
