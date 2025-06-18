#!/bin/bash

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

# Check if another fw upgrade is ongoing
check_fwupgrade_running

trap cleanup INT TERM QUIT EXIT

SCM_SPI="spi1.0"
SCM_SPIDEV="spidev1.0"
SCM_MTD=""
REMOVE_BIOS_VER=0

cleanup() {
    disconnect_spi
    if [ "$REMOVE_BIOS_VER" -eq 1 ]; then
        echo "Removing bios cache file ..."
        rm -f "$BIOS_VER_CACHE"
    fi
}

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
    echo "$program <OP> <bios file> [--partition <partition>]"
    echo "      <OP> : read, write, erase, verify"
    echo "      [<partition>] : partition of layout file; defaults to total (all sections)"
    echo "                      specify image for Aboot image sections"
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

run_flashrom() {
    # Need to expand arguments from $@.
    # shellcheck disable=SC2068
    flashrom -f -p linux_mtd:dev="$SCM_MTD" $@
}

read_flash() {
    echo "Reading flash content..."
    run_flashrom "$1 -r $2" || return 1
    sleep 1
    echo "Verifying flash content..."
    run_flashrom "$1 -v $2" || return 1
    return 0
}

write_flash() {
    echo "Writing flash content..."
    run_flashrom "$1 -w $2" || return 1
    sleep 1
    echo "Verifying flash content..."
    run_flashrom "$1 -v $2" || return 1
    return 0
}

# Image partitions are non-contiguous so can't be specified by
# a single partition name.
IMAGE_PARTITIONS="normal microcode bootblock fallback"

get_partition_opts() {
    if [ "$1" == "--partition" ]; then
        if [ -z "$2" ]; then
            echo "Missing partition argument"
            usage
            exit 1
        fi
        partitions="$2"
    else
        partitions="$3"
    fi

    if [ "$partitions" == "total" ]; then
        return
    fi

    if [ "$partitions" == "image" ]; then
        partitions="$IMAGE_PARTITIONS"
    fi

    echo -n "-l $LAYOUT_FILE"
    for partition in $partitions; do
        echo -n " -i $partition"
    done
}

if [ "$1" = "erase" ]; then
    popts=$(get_partition_opts "$2" "$3" "total")
    echo "Erasing flash content ..."
    REMOVE_BIOS_VER=1
    run_flashrom "$popts -E" || exit 1
elif [ "$1" = "read" ]; then
    popts=$(get_partition_opts "$3" "$4" "total")
    retry_command 5 read_flash "$popts" "$2" || exit 1
elif [ "$1" = "verify" ]; then
    popts=$(get_partition_opts "$3" "$4" "total")
    echo "Verifying flash content..."
    retry_command 5 run_flashrom "$popts -v $2" || exit 1
elif [ "$1" = "write" ]; then
    popts=$(get_partition_opts "$3" "$4" "total")
    REMOVE_BIOS_VER=1
    retry_command 5 write_flash "$popts" "$2" || exit 1
else
    usage
    exit 1
fi
