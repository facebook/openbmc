#!/bin/bash
#
# Copyright 2014-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-rackmond
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Start Rackmon service
### END INIT INFO

#
# No need to start rackmond if the according tty device cannot be found:
#

#shellcheck disable=SC1091

. /usr/local/bin/openbmc-utils.sh
brd_type=$(wedge_full_board_type)

if [ $((brd_type)) -ge $((0x2)) ]; then
    # workaround on Respin SKU2 USB mux default isn't BMC
    # need force it to select BMC <--> RACK
    echo 1 > $SMBCPLD_SYSFS_DIR/cpld_usb_mux2_sel
    sleep 5
fi

BMC_PSU_TTY="/dev/ttyUSB0"
if ! [ -e "$BMC_PSU_TTY" ]; then
    echo "rackmond not started: $BMC_PSU_TTY does not exist!"
    exit 1
fi

runsv /etc/sv/rackmond > /dev/null 2>&1 &
sv "$1" rackmond
