#!/bin/bash
#
# Copyright 2020-present Facebook. All Rights Reserved.
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

# shellcheck disable=SC1091
# shellcheck disable=SC2039
# SC2059 is disabled to allow binary character printing using printf
# shellcheck disable=SC2059

. /usr/local/bin/openbmc-utils.sh

EWEN_CMD=0x0
EWEN_ADDR=0x300
EWDS_CMD=0x0
EWDS_ADDR=0x0
READ_CMD=0x2
WRITE_CMD=0x1

SPEED_1MHZ=0x1

DS4520_PROGRAMMED=0
WRITES_ENABLED=0

COMMAND_REG_PREFIX="oob_eeprom_cmd_"
RESPONSE_REG_PREFIX="oob_eeprom_resp_"
READ_TRIGGER_REG="oob_eeprom_resp_trigger"
EEPROM_SIZE=1024

DS4520_BUS=8
DS4520_DEV=$(( 0x50 ))

# DS4520 pullup enable register for IO_0 through IO_7
DS4520_PULL0_REG=0xf0
# DS4520 I/O control register for IO_0 through IO_7
DS4520_PULL1_REG=0xf1
# DS4520 pullup enable register for IO_8
DS4520_IO0_REG=0xf2
# DS4520 I/O control register for IO_8
DS4520_IO1_REG=0xf3

# Setting bit 0 in the DS4520 config register makes subsequent changes
# volatile. This ensures that default settings will be restored on power loss.
DS4520_CONFIG_REG=0xf4
DS4520_VOLATILE=0x1
DS4520_CONFIG_MASK=$(( 1 << 0 ))

# PULL0/IO0 bit definitions:
SC_SPI_BUF_EN_L=7
MSW_SPI_CTRL_EN_L=6
EEPROMMASTER_SEL_EN_L=5
EEPROMMASTER_SEL=4
CPUEEEPROM_SEL_EN_L=3
CPUEEEPROM_SEL=2
DS4520_IO0_MASK=$(( ( 1 << SC_SPI_BUF_EN_L ) \
                  | ( 1 << MSW_SPI_CTRL_EN_L ) \
                  | ( 1 << EEPROMMASTER_SEL_EN_L ) \
                  | ( 1 << EEPROMMASTER_SEL ) \
                  | ( 1 << CPUEEEPROM_SEL_EN_L ) \
                  | ( 1 << CPUEEEPROM_SEL ) ))

# PULL1/IO1 bit definitions:
MSW_SPI_CTRL_SEL=0
DS4520_IO1_MASK=$(( 1 << MSW_SPI_CTRL_SEL ))

CPLD_EEPROM_MODE_0=$(( ( 1 << SC_SPI_BUF_EN_L ) \
                     | ( 1 << EEPROMMASTER_SEL ) \
                     | ( 1 << CPUEEEPROM_SEL_EN_L ) ))
CPLD_EEPROM_MODE_1=$(( 1 << MSW_SPI_CTRL_SEL ))
EEPROM_MODE_0=$(( 1 << MSW_SPI_CTRL_EN_L ))
EEPROM_MODE_1=0

trap cleanup EXIT
trap handle_signal INT TERM QUIT

usage() {
    echo "Dumps or programs the OOB Eeprom"
    echo
    echo "dump [--full]"
    echo "save <filename>"
    echo "program <filename>"
    echo "read <index> [<num-words>]"
    echo "write <index> <word>"
    echo
    echo "General options:"
    echo "  -f - overrides any safety checks"
}

read_index() {
    local cmd_reg
    cmd_reg=$(( ( READ_CMD << 30 ) | ( SPEED_1MHZ << 28 ) | ( $1 << 16 ) ))
    desc="reading index $idx"
    send_cmd "$cmd_reg" "$desc" "trig_read"
}

write_index() {
    local idx=$1
    local val=$2
    local cmd_reg
    cmd_reg=$(( ( WRITE_CMD << 30 ) | ( SPEED_1MHZ << 28 ) \
              | ( idx << 16 ) | val ))
    desc="writing index $idx value $val"
    local resp_val
    local try_num=0
    while [ $try_num -lt 5 ]
    do
        resp_val=$( send_cmd "$cmd_reg" "$desc" "trig_read" ) || exit 1
        if [ $(( resp_val )) -eq $(( val )) ]
        then
            # verify the value read matches the value programmed
            read_val=$( read_index "$idx" )
            if [ $(( read_val )) = $(( val )) ]
            then
                return
            fi

            printf "Error: try %d: read value after write(" $try_num >&2
            printf "0x%x) does not match write val(0x%x)\n" "$read_val" "$val" >&2
        else
            printf "Error: try %d: response value from write(" $try_num >&2
            printf "0x%x) does not match write val(0x%x)\n" "$resp_val" "$val" >&2
        fi

        try_num=$(( try_num + 1 ))
    done

    echo "failed to write register" >&2
    exit 1
}

eeprom_disable_write() {
    echo "Write interrupted. Disabling writes." >&2
    eeprom_write_protect "disable"
}

set_cpld_eeprom_mode() {
    # Program the DS4520 to allow access to the EEPROM.
    DS4520_PROGRAMMED=1
    i2cset -y -m $DS4520_CONFIG_MASK $DS4520_BUS $DS4520_DEV $DS4520_CONFIG_REG \
                 $DS4520_VOLATILE
    i2cset -y -m $DS4520_IO0_MASK $DS4520_BUS $DS4520_DEV $DS4520_PULL0_REG \
                 $CPLD_EEPROM_MODE_0
    i2cset -y -m $DS4520_IO1_MASK $DS4520_BUS $DS4520_DEV $DS4520_PULL1_REG \
                 $CPLD_EEPROM_MODE_1
    i2cset -y -m $DS4520_IO0_MASK $DS4520_BUS $DS4520_DEV $DS4520_IO0_REG \
                 $CPLD_EEPROM_MODE_0
    i2cset -y -m $DS4520_IO1_MASK $DS4520_BUS $DS4520_DEV $DS4520_IO1_REG \
                 $CPLD_EEPROM_MODE_1
}

restore_eeprom_mode() {
    i2cset -y -m $DS4520_CONFIG_MASK $DS4520_BUS $DS4520_DEV $DS4520_CONFIG_REG \
                 $DS4520_VOLATILE
    i2cset -y -m $DS4520_IO0_MASK $DS4520_BUS $DS4520_DEV $DS4520_PULL0_REG \
                 $EEPROM_MODE_0
    i2cset -y -m $DS4520_IO1_MASK $DS4520_BUS $DS4520_DEV $DS4520_PULL1_REG \
                 $EEPROM_MODE_1
    i2cset -y -m $DS4520_IO0_MASK $DS4520_BUS $DS4520_DEV $DS4520_IO0_REG \
                 $EEPROM_MODE_0
    i2cset -y -m $DS4520_IO1_MASK $DS4520_BUS $DS4520_DEV $DS4520_IO1_REG \
                 $EEPROM_MODE_1
    DS4520_PROGRAMMED=0
}

cleanup() {
    if [ $WRITES_ENABLED -eq 1 ]
    then
        eeprom_disable_write
        WRITES_ENABLED=0
    fi

    if [ $DS4520_PROGRAMMED -eq 1 ]
    then
        restore_eeprom_mode
    fi
}

handle_signal() {
    cleanup
    exit 1
}

eeprom_write_protect() {
    local enable=$1

    local cmd cmd_addr
    if [ "$enable" = "enable" ]
    then
        cmd=$EWEN_CMD
        cmd_addr=$EWEN_ADDR
        WRITES_ENABLED=1
    else
        cmd=$EWDS_CMD
        cmd_addr=$EWDS_ADDR
    fi

    local cmd_reg
    cmd_reg=$(( ( cmd << 30 ) | ( SPEED_1MHZ << 28 ) \
           | ( cmd_addr << 16 ) ))
    desc="setting write protect to $enable"
    send_cmd "$cmd_reg" "$desc" "trig_read" > /dev/null

    if [ "$enable" = "disable" ]
    then
        WRITES_ENABLED=0
    fi
}

eeprom_init() {
    local force=$1

    # Check to see that the CPU is powered on. This restriction can
    # be removed once the CPLD supports accessing the EEPROM with
    # the CPU powered off
    if ! wedge_is_us_on
    then
        echo "The CPU needs to be powered on to access the EEPROM."
        if [ "$force" -eq 0 ]
        then
            exit 1
        fi
        echo "Force specified. Attempting to access the EEPROM."
    fi

    set_cpld_eeprom_mode

    # Clear write cycle error, if present
    i2c_set "oob_eeprom_resp_3" 0x8
    i2c_set "oob_eeprom_resp_trigger" 0x0
}

send_cmd() {
    local cmd_reg=$1
    local desc="$2"
    local trig_opt="$3"

    local cpld_reg reg_idx
    local try_num=0
    while [ $try_num -lt 5 ]
    do
        reg_idx=0
        while [ $reg_idx -lt 4 ]
        do
            cpld_reg=${COMMAND_REG_PREFIX}$(( 3 - reg_idx ))
            cpld_reg_val=$(( ( cmd_reg >> ( ( 3 - reg_idx ) * 8 ) ) & 0xff ))
            i2c_set $cpld_reg "$cpld_reg_val"
            reg_idx=$(( reg_idx + 1 ))
        done
        if [ "$trig_opt" = "trig_read" ]
        then
            i2c_set $READ_TRIGGER_REG 0x0
        fi

        get_response "$desc" && return
        if [ $? -gt 1 ]
        then
           exit 1
        fi

        printf "Send command failed on try %d\n" $try_num >&2
        try_num=$(( try_num + 1 ))
    done

    echo "Send command failed" >&2
    exit 1
}

get_response() {
    local desc=$1
    local timeout=5
    local interval=0.1
    local time_limit=0
    local reg_idx resp_val
    while true
    do
        resp_val=0
        reg_idx=0
        while [ $reg_idx -lt 4 ]
        do
            cpld_reg=${RESPONSE_REG_PREFIX}$(( 3 - reg_idx ))
            cpld_reg_val=$( i2c_get $cpld_reg )
            # Each CPLD register contains one byte of the response with the
            # MSB first and the LSB last.
            bit_offset=$(( ( 3 - reg_idx ) * 8 ))
            shifted_val=$(( cpld_reg_val << bit_offset ))
            resp_val=$(( resp_val | shifted_val ))
            reg_idx=$(( reg_idx + 1 ))
        done

        if [ $(( ( resp_val >> 27 ) & 0x1 )) -ne 0 ]
        then
            echo "Error: Write cycle error" >&2
            return 2
        fi

        if [ $(( ( resp_val >> 28 ) & 0x1 )) -ne 0 ]
        then
            echo $(( resp_val & 0xffff ))
            return 0
        fi

        if [ $time_limit -eq 0 ]
        then
            time_limit=$(( $( date +%s ) + timeout ))
        fi
        if [ "$( date +%s )" -gt $time_limit ]
        then
            break
        fi

        sleep $interval
    done

    echo "Timed out $desc" >&2
    return 1
}

i2c_set() {
    echo "$2" > "$SUPCPLD_SYSFS_DIR/$1"
}

i2c_get() {
    head -n 1 "$SUPCPLD_SYSFS_DIR/$1"
}

eeprom_read() {
    if [ $# -lt 1 ]
    then
        echo "Error: missing read index" >&2
        usage
        exit 1
    fi

    local start=$(( $1 ))
    local num_words=1
    if [ $# -gt 1 ]
    then
        num_words=$2
    fi
    local end=$(( start + num_words ))
    if [ $end -gt $EEPROM_SIZE ]
    then
        echo "Error: end index exceeds size" >&2
        exit 1
    fi

    eeprom_write_protect "disable"

    local idx=$start
    local val
    while [ $idx -lt "$end" ]
    do
        val=$( read_index $idx )
        printf "0x%x: 0x%x\n" $idx "$val"
        idx=$(( idx + 1 ))
    done
}

add_to_csum() {
    local csum=$1
    local idx=$2
    local val=$3
    csum=$(( csum + ( idx << 16 ) + val ))
    # To prevent overflowing 32 bits, modulus with bit 31.
    csum=$(( csum % 0x80000000 ))
    echo $csum
}

eeprom_dump() {
    local csum_lsb csum_msb header version

    local full="no"
    if [ $# -gt 0 ] && [ "$1" = "--full" ]
    then
        full="yes"
    fi

    eeprom_write_protect "disable"
    local calc_csum=0

    header=$( read_index 0 )
    printf "0x%x: 0x%x\n" 0 "$header"
    # As described in Broadcom's BCM53134P data sheet, the magic code
    # is in bits 15:11 of the first word in the EEPROM and the expected
    # value is 0x15.
    local magic_code=$(( header >> 11 ))
    if [ $magic_code -ne $(( 0x15 )) ]
    then
        printf "Error: magic code 0x%x does not match expected " $magic_code >&2
        printf "0x15\n" >&2
        if [ $full = "no" ]
        then
            exit 1
        fi
    fi
    calc_csum=$( add_to_csum $calc_csum 0 "$header" )

    local num_entries=$(( header & 0x3f ))
    local stream_end=$(( 1 + num_entries ))
    echo "Number of data words: $num_entries"
    local start=1
    # 3 words are reserved at the end of the image.
    # 2 are for the checksum. 1 is for the version.
    local eeprom_end=$(( EEPROM_SIZE - 3 ))
    local end=$eeprom_end
    if [ $full = "no" ]
    then
        end=$stream_end
    fi

    local idx=$start
    while [ $idx -lt "$end" ]
    do
        val=$( read_index $idx )
        printf "0x%x: 0x%x\n" $idx "$val"
        if [ $idx -lt $stream_end ]
        then
            calc_csum=$( add_to_csum "$calc_csum" $idx "$val" )
        fi
        idx=$(( idx + 1 ))
    done

    idx=$eeprom_end
    csum_lsb=$( read_index idx )
    printf "0x%x: 0x%x\n" $idx "$csum_lsb"
    idx=$(( idx + 1 ))
    csum_msb=$( read_index idx )
    printf "0x%x: 0x%x\n" $idx "$csum_msb"
    idx=$(( idx + 1 ))
    version=$( read_index idx )
    printf "0x%x: 0x%x\n" $idx "$version"
    printf "Version: %x.%x\n" $(( version >> 8 )) $(( version & 0xff ))

    csum=$(( ( csum_msb << 16 ) | csum_lsb ))
    printf "Checksum: 0x%x\n" $csum
    printf "Calculated checksum: 0x%x\n" "$calc_csum"

    if [ "$csum" -ne "$calc_csum" ]
    then
        echo "Error: Checksum is not expected value" >&2
        exit 1
    fi
}

eeprom_save() {
    if [ $# -lt 1 ]
    then
        echo "Error: missing filename" >&2
        usage
        exit 1
    fi

    local filename=$1
    if [ -f "$filename" ]
    then
        echo "Error: file $filename already exists. Remove this file." >&2
        exit 1
    fi

    touch "$filename"
    local idx=0
    local val
    while [ $idx -lt "$EEPROM_SIZE" ]
    do
        val=$( read_index $idx )
        printf "\\$( printf "%o" $(( val & 0xff )) )" >> "$filename"
        printf "\\$( printf "%o" $(( val >> 8 )) )" >> "$filename"
        if [ $(( ( idx + 1 ) % 0x80 )) -eq 0 ]
        then
            printf "Saved word 0x%x\n" "$idx"
        fi
        idx=$(( idx + 1 ))
    done
    sync
}

eeprom_write() {
    if [ $# -lt 2 ]
    then
        echo "Error: missing arguments" >&2
        usage
        exit 1
    fi

    eeprom_write_protect "enable"

    local idx=$1
    local val=$2

    write_index "$idx" "$val"
    printf "Wrote 0x%x to 0x%x\n" "$val" "$idx"

    eeprom_write_protect "disable"
}

program_file() {
    local i=0
    local idx=0
    local val=0

    while read -r c
    do
        if [ $i -eq 0 ]
        then
            val=$c
            i=1
        else
            val=$(( val | ( c << 8 ) ))
            write_index "$idx" "$val"
            printf "Wrote 0x%x to 0x%x\n" "$val" "$idx"

            idx=$(( idx + 1 ))
            i=0
        fi
    done
}

eeprom_program() {
    if [ $# -lt 1 ]
    then
        echo "Error: missing filename" >&2
        usage
        exit 1
    fi

    local filename=$1

    if [ ! -f "$filename" ]
    then
        echo "Error: could not access file $filename" >&2
        exit 1
    fi

    size=$( stat -c %s "$filename" )
    words=$(( size / 2 ))
    if [ "$words" -ne $EEPROM_SIZE ]
    then
        printf "Error: words in file(%d) does not match eeprom " "$words" >&2
        printf "size(%d)\n" $EEPROM_SIZE >&2
        exit 1
    fi

    eeprom_write_protect "enable"
    hexdump -v -e '/1 "%u\n"' "$filename" | program_file
    eeprom_write_protect "disable"
}

force=0
if [ $# -gt 0 ] && [ "$1" = "-f" ]
then
   force=1
   shift
fi

if [ $# -lt 1 ]
then
    echo "Error: missing command" >&2
    usage
    exit 1
fi

if [ ! -d "$SUPCPLD_SYSFS_DIR" ]
then
    echo "Error: Supervisor CPLD not detected" >&2
    exit 1
fi

command="$1"
shift

eeprom_init $force

case "$command" in
    dump)
        eeprom_dump "$@"
        ;;
    save)
        eeprom_save "$@"
        ;;
    program)
        eeprom_program "$@"
        ;;
    read)
        eeprom_read "$@"
        ;;
    write)
        eeprom_write "$@"
        ;;
    *)
        usage
        exit 1
        ;;
esac
