#!/bin/bash

# shellcheck disable=SC1091
# shellcheck disable=SC2012
# shellcheck disable=SC2039
. /usr/local/bin/openbmc-utils.sh

# Check if another fw upgrade is ongoing
check_fwupgrade_running

trap cleanup INT TERM QUIT EXIT

SCM_SPI="spi1.0"
SCM_SPIDEV="spidev1.0"
SCM_MTD=""
ABOOT_CONF_START=$((0x9FA000))
FLASH_SIZE=$((0x1000000))
ABOOT_CONF_SIZE=$((0x5000))
SECTION_BLOCK_SIZE=$((0x1000))

# Temp files for storing bios file.
TEMP_BIOS_IMAGE="/tmp/tmp_bios_image"
# Temp file for storing aboot_conf data
TEMP_ABOOT_CONF="/tmp/aboot_conf.bin"

DEFAULT_ABOOT_CONF="BOOT_METHOD1=IPV6_PXE,BOOT_METHOD2=LOCAL,BOOT_METHOD3=ARISTA"

cleanup() {
    disconnect_spi
    rm -f $TEMP_ABOOT_CONF $TEMP_BIOS_IMAGE
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

create_bios_image() {
    start_block="$(($1 / SECTION_BLOCK_SIZE))"
    num_blocks="$(($2 / SECTION_BLOCK_SIZE))"
    rm -f "$TEMP_BIOS_IMAGE"
    dd if="$3" of="$TEMP_BIOS_IMAGE" bs="$SECTION_BLOCK_SIZE" count="$start_block" \
       2> /dev/null
    dd if="$4" bs="$SECTION_BLOCK_SIZE" count="$num_blocks" >> "$TEMP_BIOS_IMAGE" \
       2> /dev/null
    end_blocks=$(( (FLASH_SIZE / SECTION_BLOCK_SIZE) - start_block - num_blocks ))
    dd if="$3" bs="$SECTION_BLOCK_SIZE" count="$end_blocks" skip=$((start_block + num_blocks)) \
       >> "$TEMP_BIOS_IMAGE" 2> /dev/null
}

create_aboot_conf() {
    echo "Using Aboot conf: $1"

    nvs="$1,"

    rm -f "$TEMP_ABOOT_CONF"
    touch "$TEMP_ABOOT_CONF"
    while [ -n "${nvs%%,*}" ]; do
        nv="${nvs%%,*}"
        name="${nv%%=*}"
        if [ "$name" = "$nv" ]; then
            echo "Invalid name-value argument $nv" >&2
            return 1
        fi
        echo -n "${nv%%=*}=" >> "$TEMP_ABOOT_CONF"
        echo -n "${nv#*=}" | base64 >> "$TEMP_ABOOT_CONF"
        nvs="${nvs#*,}"
    done

    size="$(ls -l "$TEMP_ABOOT_CONF" | awk '{print $5}')"
    pad_size="$((ABOOT_CONF_SIZE - size))"
    dd if=/dev/zero bs=1 count="$pad_size" >> "$TEMP_ABOOT_CONF" 2> /dev/null
}

aboot_version() {
    grep -a CONFIG_LOCALVERSION "$1" 2> /dev/null | awk -F'"' '{print $2}'
}

if [ "$1" = "erase" ]; then
    echo "Erasing flash content ..."
    flashrom -f -p linux_mtd:dev="$SCM_MTD" -E || exit 1
elif [ "$1" = "read" ]; then
    retry_command 5 read_flash "$2" || exit 1
elif [ "$1" = "verify" ]; then
    bios_image="$2"
    if [ -n "$(aboot_version "$bios_image")" ]; then
        create_aboot_conf "$DEFAULT_ABOOT_CONF" || exit 1
        create_bios_image "$ABOOT_CONF_START" "$ABOOT_CONF_SIZE" \
                          "$bios_image" "$TEMP_ABOOT_CONF" || exit 1
        bios_image="$TEMP_BIOS_IMAGE"
    fi
    echo "Verifying flash content..."
    flashrom -f -p linux_mtd:dev="$SCM_MTD" -v "$bios_image" || exit 1
elif [ "$1" = "write" ]; then
    bios_image="$2"
    if [ -n "$(aboot_version "$bios_image")" ]; then
        create_aboot_conf "$DEFAULT_ABOOT_CONF" || exit 1
        create_bios_image "$ABOOT_CONF_START" "$ABOOT_CONF_SIZE" \
                          "$bios_image" "$TEMP_ABOOT_CONF" || exit 1
        bios_image="$TEMP_BIOS_IMAGE"
    fi
    echo "Writing flash content..."
    flashrom -n -f -p linux_mtd:dev="$SCM_MTD" -w "$bios_image" || exit 1
    sleep 1
    echo "Verifying flash content..."
    # Retry verification up to 5 times
    retry_command 5 flashrom -f -p linux_mtd:dev="$SCM_MTD" -v "$bios_image" || exit 1
else
    usage
fi
