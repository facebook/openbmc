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
    gpio_set_value CPU_JTAG_SEL 1
    gpio_set_value BMC_SPI_2_CPLD 0
}

connect_scm_jtag() {
    gpio_set_value CPU_JTAG_SEL 0
    gpio_set_value BMC_SPI_2_CPLD 1
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
   *)
      usage
      ;;
esac
