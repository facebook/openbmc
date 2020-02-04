#!/bin/sh

source /usr/local/bin/openbmc-utils.sh

CPLD_JTAG_SEL_L="CPLD_JTAG_SEL_L"
JTAG_EN="${SUPCPLD_SYSFS_DIR}/jtag_en"
JTAG_SEL="${SUPCPLD_SYSFS_DIR}/jtag_sel"
JTAG_LC_FAN_CTRL="${SCDCPLD_SYSFS_DIR}/jtag_ctrl"
JTAG_FAN_WP="${SCDCPLD_SYSFS_DIR}/fancpld_wp"

LINECARD=0

trap disconnect_jtag INT TERM QUIT EXIT

usage() {
    program=$(basename "$0")
    echo "Usage:"
    echo "$program <fpga> <action> <fpga file>"
    echo "      <fpga> : sup, scd, lc1, lc2, ..., lc8, fan"
    echo "      <action> : program, verify"
    exit 1
}

disconnect_jtag() {
    gpio_set_value $CPLD_JTAG_SEL_L 1
    echo 0 > "$JTAG_EN"
    echo 0 > "$JTAG_SEL"
    # enable fancpld write protection
    echo 1 > "$JTAG_FAN_WP"
    # do not select any linecard or fan
    echo 0x0 > "$JTAG_LC_FAN_CTRL"
    if [ $LINECARD -ne 0 ]; then
        # cycle the linecard FPGA
        gpio_set_value LC${LINECARD}_FAST_JTAG_EN 0
        gpio_set_value LC${LINECARD}_SCD_CONFIG_L 0
        sleep 1
        gpio_set_value LC${LINECARD}_SCD_CONFIG_L 1
    fi
}

connect_scd_jtag() {
    gpio_set_value $CPLD_JTAG_SEL_L 1
    echo 1 > "$JTAG_EN"
    echo 1 > "$JTAG_SEL"
}

connect_sup_jtag() {
    gpio_set_value $CPLD_JTAG_SEL_L 0
    echo 0 > "$JTAG_EN"
}

connect_linecard_jtag() {
    lc=$1
    LINECARD=${lc}
    gpio_set_value $CPLD_JTAG_SEL_L 1
    echo 1 > "$JTAG_EN"
    echo 0 > "$JTAG_SEL"
    # choose correct LC
    echo "${lc}" > "$JTAG_LC_FAN_CTRL"
    gpio_set_value LC"${lc}"_FAST_JTAG_EN 1
}

connect_fan_jtag() {
    gpio_set_value $CPLD_JTAG_SEL_L 1
    echo 1 > "$JTAG_EN"
    echo 0 > "$JTAG_SEL"
    echo 0xa > "$JTAG_LC_FAN_CTRL"
}

do_scd() {
    # verify the fpga file
    if ! grep "DESIGN.*scd" "$2" > /dev/null; then
        echo "$2 is not a vaild SCD FPGA file"
        exit 1
    fi
    connect_scd_jtag
    jam -l/usr/lib/libcpldupdate_dll_ast_jtag.so -v -a"${1^^}" "$2"
}

do_sup() {
    # verify the fpga file
    if ! grep "DESIGN.*son" "$2" > /dev/null; then
        echo "$2 is not a vaild SUP FPGA file"
        exit 1
    fi
    connect_sup_jtag
    jam -l/usr/lib/libcpldupdate_dll_ast_jtag.so -v -a"${1^^}" "$2"
}

do_fan() {
    echo "Fan CPLD upgrade is not supported"
    exit 1
}

do_linecard() {
    lc="$1"
    action="${2,,}"

    if [ "$action" != "program" ]; then
        echo "Only 'program' action is supported for linecard"
        exit 1
    fi

    if [ ${#lc} -ne 3 ]; then
        echo "'$lc' is not a valid linecard. Expect 'lc1', 'lc2', ..., 'lc8'"
        exit 1
    fi

    lc=${lc:2:1}
    if [[ $lc -lt 1 || $lc -gt 8 ]]; then
        echo "'$lc' is not a valid linecard. Expect 'lc1', 'lc2', ..., 'lc8'"
        exit 1
    fi

    connect_linecard_jtag "$lc"

    xapp -l /usr/lib/libcpldupdate_dll_ast_jtag.so "$3"
}

if [ $# -ne 3 ]; then
    usage
fi

if uname -r | grep "4\.1\.*" > /dev/null 2>&1; then
  modprobe ast_jtag
fi

if [ "$1" = "scd" ]; then
    shift 1
    do_scd "$@"
elif [ "$1" = "sup" ]; then
    shift 1
    do_sup "$@"
elif [ "$1" = "fan" ]; then
    shift 1
    do_fan "$@"
elif [[ "$1" = "lc"* ]]; then
    do_linecard "$@"
else
    usage
fi
