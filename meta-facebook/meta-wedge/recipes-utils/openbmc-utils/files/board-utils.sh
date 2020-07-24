#!/bin/sh
# Copyright 2014-present Facebook. All Rights Reserved.

#
# Functions to enable/disable isolation buffer:
#
# An Isolation Buffer is a 8-bit bus switch (TI SN74CBT3245APWR):
#  - when OE (controlled by ISO_BUF_EN GPIO) is low, 8 input ports are
#    connected to the 8 output ports.
#  - when OE (controlled by ISO_BUF_EN GPIO) is high, input and output
#    prots are disconnected.
#
# For more details, please refer to:
#  - wedge schematics, "BMC SIGNAL ISOLATION BUFFERS" in page 61.
#  - https://www.ti.com/store/ti/en/p/product/?p=SN74CBT3245APWR
#
wedge_iso_buf_enable() {
    gpio_set_value ISO_BUF_EN 0
}

wedge_iso_buf_disable() {
    gpio_set_value ISO_BUF_EN 1
}

wedge_is_us_on() {
    local val n retries prog
    if [ $# -gt 0 ]; then
        retries="$1"
    else
        retries=1
    fi
    if [ $# -gt 1 ]; then
        prog="$2"
    else
        prog=""
    fi
    if [ $# -gt 2 ]; then
        default=$3              # value 0 means defaul is 'ON'
    else
        default=1
    fi
    n=1
    while true; do
        val=$(cat "${SYSFS_I2C_DEVICES}/4-0040/gpio_inputs" 2>/dev/null)
        if [ -n "$val" ]; then
            break
        fi
        n=$((n+1))
        if [ $n -gt $retries ]; then
            echo -n " failed to read GPIO. "
            return $default
            break
        fi
        echo -n "$prog"
        sleep 1
    done
    if [ "$((val & (0x1 << 14)))" != "0" ]; then
        # powered on already
        return 0
    else
        return 1
    fi
}


# Return the board type, 'LC', 'FC-LEFT', 'FC-RIGHT', or, 'WEDGE'
wedge_board_type() {
    local pn
    pn=$(/usr/bin/weutil 2> /dev/null | grep -i '^Location on Fabric:')
    case "$pn" in
        *LEFT*)
            echo 'FC-LEFT'
            ;;
        *RIGHT*)
            echo 'FC-RIGHT'
            ;;
        *LC*)
            echo 'LC'
            ;;
        *)
            echo 'WEDGE'
            ;;
    esac
}

# On FC,
#   board rev < 2:
#     FAB_SLOT_ID (GPIOU0), low == FC0; high == FC1
#   else:
#     FAB_SLOT_ID (GPIOU6), low == FC0; high == FC1
## On LC, Wedge,
#   board rev < 3:
#     GPIOU0(ID0), GPIOU1(ID1), GPIOU2(ID2), GPIOU3(ID3)
#   else:
#     GPIOU6(ID0), GPIOU7(ID1), GPIOV0(ID2), GPIOV1(ID3)
#
# ID[2:0] ID3 Slot#
# 000      0   1
# 000      1   2
# 001      0   3
# 001      1   4
# 010      0   5
# 010      1   6
# 011      0   7
# 011      1   8

wedge_slot_id() {
    local type slot id3 id2 id1 id0 FC_CARD_BASE board_rev
    FC_CARD_BASE=100
    # need to check the board rev
    board_rev=$(wedge_board_rev)
    case "$1" in
        FC-LEFT|FC-RIGHT)
            # On FC
            slot=$(gpio_get_value FAB_SLOT_ID)
            if [ "$1" = "FC-LEFT" ]; then
                # fabric card left
                slot=$((FC_CARD_BASE * (slot + 1) + 1))
            else
                # fabric card right
                slot=$((FC_CARD_BASE * (slot + 1) + 2))
            fi
            ;;
        *)
            # either edge or LC
            id0=$(gpio_get_value BP_SLOT_ID0)
            id1=$(gpio_get_value BP_SLOT_ID1)
            id2=$(gpio_get_value BP_SLOT_ID2)
            id3=$(gpio_get_value BP_SLOT_ID3)
            slot=$(((id2 * 4 + id1 * 2 + id0) * 2 + id3 + 1))
    esac
    echo "$slot"
}

# wedge_board_rev() is only valid after GPIO Y0, Y1, and Y2 are enabled
wedge_board_rev() {
    local val0 val1 val2
    val0=$(gpio_get_value BOARD_REV_ID0)
    val1=$(gpio_get_value BOARD_REV_ID1)
    val2=$(gpio_get_value BOARD_REV_ID2)
    echo $((val0 | (val1 << 1) | (val2 << 2)))
}

# Should we enable OOB interface or not
wedge_should_enable_oob() {
    board_rev=$(wedge_board_rev)
    board_type=$(wedge_board_type)
    case "$board_type" in
        FC-LEFT|FC-RIGHT)
            if [ $board_rev -lt 2 ]; then
                return 0
            fi
            ;;
        *)
            if [ $board_rev -lt 3 ]; then
                return 0
            fi
            ;;
    esac
    return 1
}
