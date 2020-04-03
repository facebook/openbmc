#!/bin/sh

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh


trap disconnect_spi INT TERM QUIT EXIT

usage() {
    program=$(basename "$0")
    echo "Usage:"
    echo "$program <OP> <bios file>"
    echo "      <OP> : read, write, erase, recover"
    exit 1
}

disconnect_spi() {
    # connect through CPLD
    echo 0x0 > "${SUPCPLD_SYSFS_DIR}"/bios_select
}

connect_spi() {
    # spi2 cs0
    for bit in 26 27 28 29; do
        if ! devmem_test_bit "$(scu_addr 88)" $bit; then
           echo "pinctrl error: SPI2 function bits not enabled!"
           exit 1
        fi
     done

    # connect through CPLD
    echo 0x1 > "${SUPCPLD_SYSFS_DIR}"/bios_select
}

# Depending on the flash source, we might need to retry the command without the -c option

do_erase() {
    echo "Erasing flash content ..."
    if ! flashrom -p linux_spi:dev=/dev/spidev2.0 -E -c "MX25L12835F/MX25L12845E/MX25L12865E"; then
      echo "flashrom failed. Retrying without -c"
      if ! flashrom -p linux_spi:dev=/dev/spidev2.0 -E; then
         echo "flashrom without -c also failed"
      fi
    fi
}

do_read() {
    echo "Reading flash content..."
    if ! flashrom -p linux_spi:dev=/dev/spidev2.0 -r "$1" -c "MX25L12835F/MX25L12845E/MX25L12865E"; then
      echo "flashrom failed. Retrying without -c"
      if ! flashrom -p linux_spi:dev=/dev/spidev2.0 -r "$1"; then
         echo "flashrom without -c also failed"
      fi
    fi
}

do_write() {
    echo "Writing flash content..."
    if ! flashrom -p linux_spi:dev=/dev/spidev2.0 -w "$1" -c "MX25L12835F/MX25L12845E/MX25L12865E"; then
      echo "flashrom failed. Retrying without -c"
      if ! flashrom -p linux_spi:dev=/dev/spidev2.0 -w "$1"; then
         echo "flashrom without -c also failed"
      fi
    fi
}

connect_spi

if [ "$1" = "erase" ]; then
    do_erase
elif [ "$1" = "read" ]; then
    do_read "$2"
elif [ "$1" = "write" ]; then
    do_write "$2"
else
    usage
fi
