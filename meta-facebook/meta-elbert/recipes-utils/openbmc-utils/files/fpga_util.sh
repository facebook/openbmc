#!/bin/sh

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

CPLD_JTAG_SEL_L="CPLD_JTAG_SEL_L"
JTAG_EN="${SUPCPLD_SYSFS_DIR}/jtag_en"
JTAG_SEL="${SUPCPLD_SYSFS_DIR}/jtag_sel"

# ELBERTTODO SCD, LC, FAN support

trap disconnect_program_paths INT TERM QUIT EXIT

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
}

disconnect_program_paths() {
   disconnect_jtag
}

connect_sup_jtag() {
    gpio_set_value $CPLD_JTAG_SEL_L 0
    echo 0 > "$JTAG_EN"
}

do_sup() {
    # verify the fpga file
    if ! grep "DESIGN.*ird" "$2" > /dev/null; then
        echo "$2 is not a vaild SUP FPGA file"
        exit 1
    fi
    connect_sup_jtag
    jam -l/usr/lib/libcpldupdate_dll_ast_jtag.so -v -a"${1^^}" "$2"
}

do_scd() {
    echo "SCD CPLD upgrade is not supported"
    exit 1
}

do_fan() {
    echo "Fan CPLD upgrade is not supported"
    exit 1
}

do_linecard() {
   echo "LC CPLD Upgrade is not supported"
   exit 1
}

if [ $# -ne 3 ]; then
    usage
fi

case "$1" in
   scd) shift 1
      do_scd "$@"
      ;;
   sup) shift 1
      do_sup "$@"
      ;;
   lc*) shift 1
      do_linecard "$@"
      ;;
   fan) shift 1
      do_fan "$@"
      ;;
   *)
      usage
      ;;
esac
