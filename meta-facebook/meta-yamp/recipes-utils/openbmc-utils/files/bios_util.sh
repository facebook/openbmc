#!/bin/sh

source /usr/local/bin/openbmc-utils.sh

trap disconnect_spi INT TERM QUIT EXIT

usage() {
    program=`basename "$0"`
    echo "Usage:"
    echo "$program <OP> <bios file>"
    echo "      <OP> : read, write, erase"
    exit -1
}

disconnect_spi() {
    # connect through CPLD
    echo 0x0 > ${SUPCPLD_SYSFS_DIR}/bios_select
}

connect_spi() {
    # spi2 cs0
    devmem_set_bit $(scu_addr 88) 26
    devmem_set_bit $(scu_addr 88) 27
    devmem_set_bit $(scu_addr 88) 28
    devmem_set_bit $(scu_addr 88) 29

    # connect through CPLD
    echo 0x1 > ${SUPCPLD_SYSFS_DIR}/bios_select
}

do_erase() {
    flashrom -p linux_spi:dev=/dev/spidev2.0 -E -c "MX25L12835F/MX25L12845E/MX25L12865E"
}

do_read() {
    flashrom -p linux_spi:dev=/dev/spidev2.0 -r "$1" -c "MX25L12835F/MX25L12845E/MX25L12865E"
}

do_write() {
    flashrom -p linux_spi:dev=/dev/spidev2.0 -w "$1" -c "MX25L12835F/MX25L12845E/MX25L12865E"
}

connect_spi

if [ "$1" == "erase" ]; then
    do_erase
elif [ "$1" == "read" ]; then
        do_read "$2"
elif [ "$1" == "write" ]; then
        do_write "$2"
else
    usage
fi
