#!/bin/bash

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

# Check if another fw upgrade is ongoing
check_fwupgrade_running

trap cleanup INT TERM QUIT EXIT

SPI_CHANNEL="1.0"
DRIVER=""

usage() {
    program=$(basename "$0")
    echo "Usage:"
    echo "$program <action> [<bios file>] [--partition <partition>]"
    echo "      <action> : read, write, verify, erase, fullread, fullwrite"
    echo "      [<bios file>] : file to read to/write from"
    echo "          Required for all actions except erase"
    echo "      [<partition>] : partition of layout file; defaults to image"
    echo "          If <partition> is given, <action> must be"
    echo "          read, write, program, verify, or erase"
    exit 1
}

cleanup() {
    disconnect_program_paths
}

disconnect_program_paths() {
    unbind_spi_nor_driver "$SPI_CHANNEL"
    gpio_set_value ABOOT_GRAB 0
    gpio_set_value SW_CPLD_JTAG_SEL 0
    gpio_set_value SW_JTAG_SEL 0
    gpio_set_value SW_SPI_SEL 0
    userver_power_on
}

connect_spi() {
    devmem_clear_bit 0x1e6e2438 8
    # Take CPU into reset via power and reset control
    i2cset -f -y 14 0x28 0x2e 0x00
    echo 0 > /sys/bus/i2c/drivers/pwrcpld/12-0043/cpu_control
    sleep 0.5
    gpio_set_value ABOOT_GRAB 1
    gpio_set_value SW_CPLD_JTAG_SEL 0
    gpio_set_value SW_JTAG_SEL 0
    gpio_set_value SW_SPI_SEL 0
}

select_spi_driver() {
    # Select the SPI driver
    # For GD25LQ - we want to use flashrom, otherwise spinor
    unbind_spi_nor_driver "$SPI_CHANNEL"
    if do_spi_image "gd25lq256d" "FLASHROM" "$SPI_CHANNEL" "BIOS" "image" "IDENTIFY"; then
        DRIVER="FLASHROM"
        unbind_spi_nor_driver "$SPI_CHANNEL"
    else
        DRIVER="SPINOR"
        bind_spi_nor_driver "$SPI_CHANNEL"
    fi
}

narg_err() {
    echo "Invalid number of arguments"
    usage
    exit 1
}

case "${1^^}" in
    FULLREAD|FULLWRITE)
        if [ $# -ne 2 ]; then
            narg_err
        fi
        partition="total"
        target_image="$2"
        ;;
    READ|VERIFY|WRITE|PROGRAM)
        if [ $# -eq 2 ]; then
            partition="image"
            target_image="$2"
        elif [ $# -eq 4 ] && [ "$3" == "--partition" ]; then
            partition="$4"
            target_image="$2"
        else
            narg_err
        fi
        ;;
    ERASE)
        if [ $# -eq 1 ]; then
            partition="image"
        elif [ $# -eq 3 ] && [ "$2" == "--partition" ]; then
            partition="$3"
        else
            narg_err
        fi
        target_image=""
        ;;
    *)
        echo "Unknown action: $1"
        usage
        ;;
esac

connect_spi
select_spi_driver
do_spi_image "$target_image" "$DRIVER" "$SPI_CHANNEL" "BIOS" "$partition" "$1"
