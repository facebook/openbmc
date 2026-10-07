#!/bin/bash
#
# Copyright 2026-present Facebook. All Rights Reserved.
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

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

# shellcheck disable=SC2039
usage() {
    echo "Reads/writes Bcm53134 registers"
    echo
    echo "read[(8|16|32|64)] [-f] <page> <addr>"
    echo "write[(8|16|32|64)] [-f] <page> <addr> <val>"
    echo "mii-read <port-addr> <addr>"
    echo "mii-write <port-addr> <addr> <val>"
    echo
    echo "read/write are equivalent to read8/write8"
    echo
    echo "Return code:"
    echo "   0 on successful read or write"
    echo "   1 on failed read or write"
    echo "   2 when the command is disabled. For certain hardware, the read/write "
    echo "     commands are disabled due to not-recommended MDIO bus transactions"
}

# The AST2700 MDIO controller is enabled and pin-muxed by the kernel device
# tree. Unlike Meru's AST2600 implementation, no mdio_init(), mdio_restore(),
# cleanup(), or signal trap is needed: the kernel owns controller reset and
# pin-mux configuration for the lifetime of the system.

mdio0_base_reg=0x14040000
mdio_ctrl_reg=0x0
mdio_read_reg=0x4
pseudo_phy_port_addr=0x1e

to_hex() {
    local val=$1
    printf "0x%x" "$val"
}

devmem_read() {
    local reg_addr=$1
    local mem_addr

    mem_addr=$(to_hex $((mdio0_base_reg + reg_addr)))
    devmem "$mem_addr"
}

devmem_write() {
    local reg_addr=$1
    local val mem_addr

    val=$(to_hex "$2")
    mem_addr=$(to_hex $((mdio0_base_reg + reg_addr)))
    devmem "$mem_addr" 32 "$val"
}

mdio_read() {
    local port_addr=$1
    local addr=$2
    local fire_busy clause_22 read_request reg_val read_reg_val ctrl_idle

    fire_busy=$(( 1 << 31 ))
    clause_22=$(( 1 << 28 ))
    read_request=$(( 1 << 27 ))
    reg_val=$(( fire_busy \
              | clause_22 \
              | read_request \
              | ( port_addr << 21 ) \
              | ( addr << 16 ) ))

    devmem_write $mdio_ctrl_reg $reg_val
    ctrl_idle=$(( 1 << 16 ))
    desc=$(printf "read 0x%x" "$addr")
    mdio_wait_op_complete "$mdio_read_reg" "$ctrl_idle" "$ctrl_idle" "$desc"

    read_reg_val=$(devmem_read $mdio_read_reg)
    echo $(( read_reg_val & 0xffff ))
}

mdio_write() {
    local port_addr=$1
    local addr=$2
    local val=$3
    local fire_busy clause_22 write_request reg_val

    fire_busy=$(( 1 << 31 ))
    clause_22=$(( 1 << 28 ))
    write_request=$(( 1 << 26 ))
    reg_val=$(( fire_busy \
              | clause_22 \
              | write_request \
              | ( port_addr << 21 ) \
              | ( addr << 16 ) \
              | val ))

    devmem_write $mdio_ctrl_reg "$reg_val"

    desc=$(printf "write 0x%x 0x%x" "$addr" "$val")
    mdio_wait_op_complete "$mdio_ctrl_reg" "$fire_busy" 0 "$desc"
}

mdio_wait_op_complete() {
    local mdio_reg=$1
    local mask=$2
    local wait_val=$3
    local desc=$4
    local timeout=5
    local interval=0.1
    local val

    local time_limit=$(( $(date +%s) + timeout ))
    while [ "$(date +%s)" -lt $time_limit ]
    do
        val=$(devmem_read "$mdio_reg")
        if [ $(( val & "$mask" )) -eq "$wait_val" ]
        then
            return
        fi
        sleep $interval
    done

    echo "Error: timed out waiting for operation to complete: $desc" >&2
    exit 1
}

oob_page_reg=16
oob_addr_reg=17
oob_data_reg_base=24

reg_read() {
    local page=$1
    local addr=$2
    local data_size=$3
    local data data_reg_num reg_val

    data=$(reg_page_data "$page" 1)
    mdio_write $pseudo_phy_port_addr $oob_page_reg "$data"
    page_reg_val=$(mdio_read $pseudo_phy_port_addr $oob_page_reg)
    if [ "$page_reg_val" -ne "$data" ]; then
        echo "Error: page register could not be written" >&2
        exit 1
    fi

    data=$(reg_addr_data "$addr" 0x2)
    mdio_write $pseudo_phy_port_addr $oob_addr_reg "$data"

    reg_wait_addr_reg_acked

    # Read data registers
    local val=0
    num_data_regs=$(( ( data_size + 1 ) / 2 ))
    local reg_idx=0
    while [ $reg_idx -lt $num_data_regs ]
    do
        data_reg_num=$(( oob_data_reg_base + reg_idx ))
        reg_val=$(mdio_read $pseudo_phy_port_addr $data_reg_num)
        val=$(( val | ( reg_val << ( reg_idx * 16 ) ) ))

        reg_idx=$(( reg_idx + 1 ))
    done

    binary_str=$(val_to_binary_str "$val" "$data_size")
    printf "0x%x/0x%x 0x%x == $binary_str\n" "$page" "$addr" $val

    reg_clear_access_enable
}

reg_write() {
    local page=$1
    local addr=$2
    local val=$3
    local data_size=$4
    local data reg_val data_reg_num

    data=$(reg_page_data "$page" 1)
    mdio_write $pseudo_phy_port_addr $oob_page_reg "$data"
    page_reg_val=$(mdio_read $pseudo_phy_port_addr $oob_page_reg)
    if [ "$page_reg_val" -ne "$data" ]; then
        echo "Error: page register could not be written" >&2
        exit 1
    fi

    # Write data registers
    num_data_regs=$(( ( data_size + 1 ) / 2 ))
    local reg_idx=0
    while [ $reg_idx -lt $num_data_regs ]
    do
        data_reg_num=$(( oob_data_reg_base + reg_idx ))
        reg_val=$(( ( val >> ( reg_idx * 16 ) ) & 0xffff ))
        mdio_write $pseudo_phy_port_addr $data_reg_num $reg_val

        reg_idx=$(( reg_idx + 1 ))
    done

    data=$(reg_addr_data "$addr" 0x1)
    mdio_write $pseudo_phy_port_addr $oob_addr_reg "$data"

    reg_wait_addr_reg_acked
    reg_clear_access_enable
}

reg_wait_addr_reg_acked() {
    local timeout=60
    local interval=0.1
    local time_limit response op

    time_limit=$(( $(date +%s) + timeout ))
    while [ "$(date +%s)" -lt $time_limit ]
    do
        response=$(mdio_read $pseudo_phy_port_addr $oob_addr_reg)
        op=$(( response & 0x3 ))
        if [ $op -eq 0 ]
        then
            return
        fi

        sleep $interval
    done

    echo "Error: timed out waiting for addr reg to be acknowledged" >&2
    exit 1
}

reg_clear_access_enable() {
    data=$(reg_page_data 0 0)
    mdio_write $pseudo_phy_port_addr $oob_page_reg "$data"
}

reg_page_data() {
    local page=$1
    local access_enable=$2
    local data=$(( page << 8 ))
    data=$(( data | access_enable ))
    echo $data
}

reg_addr_data() {
    local addr=$1
    local op=$2
    local data=$(( addr << 8 ))
    data=$(( data | op ))
    echo $data
}

val_to_binary_str() {
    local val=$1
    local data_size=$2
    local i=$(( data_size * 8 ))

    while [ $i -gt 0 ]
    do
        i=$(( i - 1 ))
        printf "%d" $(( ( val >> i ) & 0x1 ))
        if [ $i -gt 0 ] && [ $(( i % 4 )) -eq 0 ]
        then
            printf " "
        fi
    done
}

handle_read() {
    local size=$1
    shift

    if [ $# -lt 2 ]
    then
        echo "Error: missing arguments" >&2
        usage
        exit 1
    fi
    local page=$1
    local addr=$2
    local data_size=$(( size / 8 ))

    reg_read "$page" "$addr" "$data_size"
}

handle_write() {
    local size=$1
    shift

    if [ $# -lt 3 ]
    then
        echo "Error: missing arguments" >&2
        usage
        exit 1
    fi
    local page=$1
    local addr=$2
    local val=$3
    local data_size=$(( size / 8 ))

    reg_write "$page" "$addr" "$val" "$data_size"
}

handle_mii_read() {
    if [ $# -lt 2 ]
    then
        echo "Error: missing arguments" >&2
        usage
        exit 1
    fi
    local port_addr=$1
    local addr=$2

    val=$(mdio_read "$port_addr" "$addr")
    printf "0x%x\n" "$val"
}

handle_mii_write() {
    if [ $# -lt 3 ]
    then
        echo "Error: missing arguments" >&2
        usage
        exit 1
    fi
    local port_addr=$1
    local addr=$2
    local val=$3

    mdio_write "$port_addr" "$addr" "$val"
}

if [ $# -lt 1 ]
then
    echo "Error: missing command" >&2
    usage
    exit 1
fi

command="$1"
shift
case "$command" in
    read8) cmd_type="read"; size="8";;
    read16) cmd_type="read"; size="16";;
    read32) cmd_type="read"; size="32";;
    read64) cmd_type="read"; size="64";;
    write8) cmd_type="write"; size="8";;
    write16) cmd_type="write"; size="16";;
    write32) cmd_type="write"; size="32";;
    write64) cmd_type="write"; size="64";;
    mii-read) ;;
    mii-write) ;;
    *)
        usage
        exit 1
        ;;
esac

if [ "$command" = "mii-read" ]; then
    handle_mii_read "$@"
elif [ "$command" = "mii-write" ]; then
    handle_mii_write "$@"
elif [ "$cmd_type" = "read" ]; then
    handle_read "$size" "$@"
else
    handle_write "$size" "$@"
fi
