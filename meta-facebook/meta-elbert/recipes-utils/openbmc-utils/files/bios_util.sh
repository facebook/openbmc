#!/bin/sh

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

# Check if another fw upgrade is ongoing
check_fwupgrade_running

trap disconnect_spi INT TERM QUIT EXIT

SCM_SPI="spi1.0"
SCM_SPIDEV="spidev1.0"
SCM_MTD=""

bind_spi_nor() {
    # Unbind spidev
    if [ -e /dev/"$SCM_SPIDEV" ]; then
        echo "$SCM_SPI" > /sys/bus/spi/drivers/spidev/unbind
    fi

    # Bind
    echo spi-nor  > /sys/bus/spi/devices/"$SCM_SPI"/driver_override
    if [ ! -e /sys/bus/spi/drivers/spi-nor/"$SCM_SPI" ]; then
        echo Binding "$SCM_SPI" to ...
        echo "$SCM_SPI" > /sys/bus/spi/drivers/spi-nor/bind
        sleep 0.5
    fi
    SCM_MTD="$(grep "$SCM_SPI" /proc/mtd |awk '{print$1}' | tr -d : | tr -d mtd)"
    if test -z "$SCM_MTD"; then
        echo "Failed to locate mtd partition for SPI Flash!"
        exit 1
    fi
}

unbind_spi_nor() {
    # Method for unloading spi-nor driver dynamically back to spidev
    echo > /sys/bus/spi/devices/"$SCM_SPI"/driver_override
    if grep "$SCM_SPI" /proc/mtd > /dev/null 2>&1 ; then
        echo "Unbinding spi-nor from $SCM_SPI"
        echo "$SCM_SPI" > /sys/bus/spi/drivers/spi-nor/unbind
    fi
    if [ ! -e /dev/"$SCM_SPIDEV" ]; then
        echo "Binding spidev back to $SCM_SPI"
        echo "$SCM_SPI" > /sys/bus/spi/drivers/spidev/bind
    fi
}

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
    unbind_spi_nor
}

connect_spi() {
    # Set GPIOV0 to 0
    gpio_set_value BMC_SPI1_CS0_MUX_SEL 0
    # connect through CPLD
    echo 0x1 > "${SCMCPLD_SYSFS_DIR}"/bios_select
    bind_spi_nor
}

connect_spi

read_flash() {
    echo "Reading flash content..."
    flashrom -f -p linux_mtd:dev="$SCM_MTD" -r "$1" || return 1
    sleep 1
    echo "Verifying flash content..."
    flashrom -f -p linux_mtd:dev="$SCM_MTD" -v "$1" || return 1
    return 0
}

if [ "$1" = "erase" ]; then
    echo "Erasing flash content ..."
    flashrom -f linux_mtd:dev="$SCM_MTD" -E || exit 1
elif [ "$1" = "read" ]; then
    retry_command 5 read_flash "$2" || exit 1
elif [ "$1" = "verify" ]; then
    echo "Verifying flash content..."
    flashrom -f -p linux_mtd:dev="$SCM_MTD" -v "$2" || exit 1
elif [ "$1" = "write" ]; then
    echo "Writing flash content..."
    flashrom -n -f -p linux_mtd:dev="$SCM_MTD" -w "$2" || exit 1
    sleep 1
    echo "Verifying flash content..."
    # Retry verification up to 5 times
    retry_command 5 flashrom -f -p linux_mtd:dev="$SCM_MTD" -v "$2" || exit 1
else
    usage
fi
