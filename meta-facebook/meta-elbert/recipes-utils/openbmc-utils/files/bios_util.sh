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
    echo 0x0 > "${SCMCPLD_SYSFS_DIR}"/bios_select
    # Set GPIOV0 to 0 (default GPIO state)
    gpio_set_value BMC_SPI1_CS0_MUX_SEL 0
}

connect_spi() {
    # Set GPIOV0 to 0
    gpio_set_value BMC_SPI1_CS0_MUX_SEL 0
    # connect through CPLD
    echo 0x1 > "${SCMCPLD_SYSFS_DIR}"/bios_select
}

# Depending on the flash source, we might need to retry the command without the -c option

do_erase() {
    echo "Erasing flash content ..."
    if ! flashrom -p linux_spi:dev=/dev/spidev1.0 -E -c "MX25L12835F/MX25L12845E/MX25L12865E"; then
      echo "flashrom failed. Retrying without -c"
      if ! flashrom -p linux_spi:dev=/dev/spidev1.0 -E; then
         echo "flashrom without -c also failed"
      fi
    fi
}

do_read() {
    echo "Reading flash content..."
    if ! flashrom -p linux_spi:dev=/dev/spidev1.0 -r "$1" -c "MX25L12835F/MX25L12845E/MX25L12865E"; then
      echo "flashrom failed. Retrying without -c"
      if ! flashrom -p linux_spi:dev=/dev/spidev1.0 -r "$1"; then
         echo "flashrom without -c also failed"
      fi
    fi
}

do_write() {
    echo "Writing flash content..."
    if ! flashrom -p linux_spi:dev=/dev/spidev1.0 -w "$1" -c "MX25L12835F/MX25L12845E/MX25L12865E"; then
      echo "flashrom failed. Retrying without -c"
      if ! flashrom -p linux_spi:dev=/dev/spidev1.0 -w "$1"; then
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
