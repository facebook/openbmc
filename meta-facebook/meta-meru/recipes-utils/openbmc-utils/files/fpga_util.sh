#!/bin/bash

# shellcheck disable=SC1091
# shellcheck disable=SC2012
# shellcheck disable=SC2034
. /usr/local/bin/openbmc-utils.sh

# Check if another fw upgrade is ongoing
check_fwupgrade_running

trap cleanup INT TERM QUIT EXIT

usage() {
    program=$(basename "$0")
    echo "Usage:"
    echo "$program <fpga> <action> <fpga file>"
    echo "      <fpga> : scm"
    echo "      <action> : program, verify"
    echo "      <fpga file> : file to program/verify"
    exit 1
}

cleanup() {
    disconnect_program_paths
}

disconnect_program_paths() {
    gpio_set_value BMC_LITE_L 0
    gpio_set_value JTAG_TRST_L 0
    echo 1 > "$SCM_PWR_ON_SYSFS"
}

connect_scm_jtag() {
    echo 0 > "$SCM_PWR_ON_SYSFS"
    gpio_set_value BMC_LITE_L 1
    gpio_set_value SW_JTAG_SEL 0
    gpio_set_value JTAG_TRST_L 1
}

do_scm() {
    connect_scm_jtag
    jam -l/usr/lib/libcpldupdate_dll_ioctl.so -v -a"${1^^}" "$2" \
      --ioctl IOCBITBANG
}

if [ $# -lt 2 ]; then
    usage
fi

case "$1" in
   scm) shift 1
      case "${1^^}" in
          PROGRAM|VERIFY)
          do_scm "$@"
          exit 0
          ;;
      esac
      ;;
esac

usage
