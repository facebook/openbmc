#!/bin/sh

#shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh
brd_type=$(wedge_full_board_type)

if [ $((brd_type)) -ge $((0x2)) ] && ! [ -e /run/rackmond.env ]; then
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

# Set a default config.
INTERFACE_CONFIG_FILE="/usr/share/rackmon/interface/usb_ft232.conf"
echo "INTERFACE_CONFIG_FILE=$INTERFACE_CONFIG_FILE" > /run/rackmond.env

# Wedge400 MP Respin or newer, change to use FT4232 for rackmon uart
if [ $((brd_type)) -ge $((0x2)) ]; then

    # Detect FTDI chip
    /usr/local/bin/ftdi_control -L
    ftdi_detected=$?
    # exit when can't detect FTDI Chip
    if [ $((ftdi_detected)) -ne 0 ]; then
        echo "run-rackmond.sh : cannot detect FTDI chip" > /dev/kmsg
        exit 1
    fi

    # set FT4232 port 1 to GPIO Input
    /usr/local/bin/ftdi_bitbang -I 1 -i 0 -i 1 -i 2 -i 4 -i 5 -i 6

    rs485_cfg=0
    rs485_cfg=$((rs485_cfg + $(/usr/local/bin/ftdi_control -o | grep CHANNEL_B_RS485 | grep -c 1 ) ))
    rs485_cfg=$((rs485_cfg + $(/usr/local/bin/ftdi_control -o | grep CHANNEL_C_RS485 | grep -c 1 ) ))
    rs485_cfg=$((rs485_cfg + $(/usr/local/bin/ftdi_control -o | grep CHANNEL_D_RS485 | grep -c 1 ) ))
    if [ $((rs485_cfg)) -lt 3 ]; then
        /usr/local/bin/ftdi_control -N -B1 -C1 -D1
        touch /tmp/need_to_pwrcycle
        echo "run-rackmond.sh : Need to power cycle to reload the FTDI configuration" > /dev/kmsg
        exit 1
    fi
    if [ -e /tmp/need_to_pwrcycle ]; then
        echo "run-rackmond.sh : Need to power cycle to reload the FTDI configuration" > /dev/kmsg
        exit 1
    fi
    # Replace with multiport configuration.
    INTERFACE_CONFIG_FILE="/usr/share/rackmon/interface/usb_ft4232.conf"
else
    INTERFACE_CONFIG_FILE="/usr/share/rackmon/interface/usb_ft232.conf"
fi


echo "INTERFACE_CONFIG_FILE=$INTERFACE_CONFIG_FILE" > /run/rackmond.env

