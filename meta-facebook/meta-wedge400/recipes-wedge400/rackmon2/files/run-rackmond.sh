#!/bin/sh

#shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh
brd_type=$(wedge_full_board_type)

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
    ln -s /etc/usb_ft4232.conf /etc/rackmon.conf
else
  ln -s /etc/usb_ft232.conf /etc/rackmon.conf
fi


exec /usr/local/bin/rackmond

