#!/bin/sh

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh


trap disconnect_spi INT TERM QUIT EXIT

usage() {
    program=$(basename "$0")
    echo "Usage:"
    echo "$program <OP> <bios file>"
    echo "      <OP> : read, write, erase, verify"
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

connect_spi

# Depending on the flash source, we might need to retry the command with different -c options
if flashrom -p linux_spi:dev=/dev/spidev1.0  | grep -q "MX25L12835F/MX25L12845E/MX25L12865E"; then
    CHIPTYPE="MX25L12835F/MX25L12845E/MX25L12865E"
elif flashrom -p linux_spi:dev=/dev/spidev1.0  | grep -q "N25Q128..3E"; then
    CHIPTYPE="N25Q128..3E"
else
   echo "Unknown Flash type!"
   echo "See flashrom output below:"
   flashrom -p linux_spi:dev=/dev/spidev1.0
   exit 1
fi

if [ "$1" = "erase" ]; then
    echo "Erasing flash content ..."
    flashrom -f -p linux_spi:dev=/dev/spidev1.0 -E -c $CHIPTYPE || exit 1
elif [ "$1" = "read" ]; then
    echo "Reading flash content..."
    flashrom -p linux_spi:dev=/dev/spidev1.0 -r "$2" -c $CHIPTYPE || exit 1
elif [ "$1" = "verify" ]; then
    echo "Verifying flash content..."
    flashrom -f -p linux_spi:dev=/dev/spidev1.0 -v "$2" -c $CHIPTYPE || exit 1
elif [ "$1" = "write" ]; then
    echo "Writing flash content..."
    flashrom -n -f -p linux_spi:dev=/dev/spidev1.0 -w "$2" -c $CHIPTYPE || exit 1
    echo "Verifying flash content..."
    # ELBERTTODO understand why read is flaky, giving false verification fails
    # Retry verification up to 5 times
    retry_command 5 flashrom -f -p linux_spi:dev=/dev/spidev1.0 \
       -v "$2" -c $CHIPTYPE || exit 1
else
    usage
fi
