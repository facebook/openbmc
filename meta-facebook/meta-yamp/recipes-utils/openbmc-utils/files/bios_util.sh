#!/bin/sh

# shellcheck disable=SC1091
# shellcheck disable=SC2012
# shellcheck disable=SC2039
source /usr/local/bin/openbmc-utils.sh

trap cleanup INT TERM QUIT EXIT

ABOOT_CONF_START=$((0x9FA000))
FLASH_SIZE=$((0x1000000))
ABOOT_CONF_SIZE=$((0x5000))
SECTION_BLOCK_SIZE=$((0x1000))

# Temp files for storing bios file.
TEMP_BIOS_IMAGE="/tmp/tmp_bios_image"
# Temp file for storing aboot_conf data
TEMP_ABOOT_CONF="/tmp/aboot_conf.bin"

DEFAULT_ABOOT_CONF="BOOT_METHOD1=IPV6_PXE,BOOT_METHOD2=LOCAL,BOOT_METHOD3=ARISTA"

BIOS_SPIDEV="/dev/spidev2.0"
BIOS_CHIP="MX25L12835F/MX25L12845E/MX25L12865E"

cleanup() {
    disconnect_spi
    rm -f $TEMP_ABOOT_CONF $TEMP_BIOS_IMAGE
}

usage() {
    program=$(basename "$0")
    echo "Usage:"
    echo "$program <OP> <bios file>"
    echo "      <OP> : read, write, erase, recover"
    exit 1
}

disconnect_spi() {
    # connect through CPLD
    echo 0x0 > "${SUPCPLD_SYSFS_DIR}/bios_select"
}

connect_spi() {
    # spi2 cs0
    devmem_set_bit "$(scu_addr 88)" 26
    devmem_set_bit "$(scu_addr 88)" 27
    devmem_set_bit "$(scu_addr 88)" 28
    devmem_set_bit "$(scu_addr 88)" 29

    # connect through CPLD
    echo 0x1 > "${SUPCPLD_SYSFS_DIR}/bios_select"
}

# Arista added a 3rd source, so this function is needed in case we are dealing with the 3rd source
do_retry(){
  if [ "$1" = "write" ]; then
    if ! flashrom --layout /etc/yamp_bios.layout --image header --image payload -p linux_spi:dev="$BIOS_SPIDEV" -w "$2"; then
      echo "flashrom without -c failed"
    fi
  elif [ "$1" = "erase" ]; then
    if ! flashrom -p linux_spi:dev="$BIOS_SPIDEV" -E; then
       echo "flashrom without -c option failed as well"
    fi
  else # reading
    if ! flashrom -p linux_spi:dev="$BIOS_SPIDEV" -r "$2"; then
      echo "flashrom without -c option failed"
    fi
  fi
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
        if [ "$name" == "$nv" ]; then
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

do_erase() {
    # Layout is not supported in Erase. 
    # So we need to manually recover pdr (which include idprom) after erase.
    if [ ! -e /mnt/data/header_pdr.data ]; then
      backup_image
    fi

    # Do the erase
    echo "Erasing the flash"
    if ! flashrom -p linux_spi:dev=$BIOS_SPIDEV -E -c "$BIOS_CHIP"; then
      echo "flashrom failed. Retrying without -c"
      do_retry "erase" 
    fi
    
    # Recover header and pdr (which includes the idprom)
    do_recover
}

do_read() {
    echo "Reading flash content..."
    if ! flashrom -p linux_spi:dev="$BIOS_SPIDEV" -r "$1" -c "$BIOS_CHIP"; then
      echo "flashrom failed. Retrying without -c"
      do_retry "read" "$1"
    fi
}

backup_image(){
    echo "Backing up pdr"

    # Get complete image
    if ! tempfile=$(mktemp); then
      echo "Running mktemp in backup_image failed"
      exit 1
    fi
    
    # /mnt/data should always exist
    if [ ! -d /mnt/data ]; then
      echo "Partition /mnt/data doesn't exist"
      exit 1
    fi
    
    # Create /mnt/data fixed name which will be used to recover later
    header_pdr_file="/mnt/data/header_pdr.data"

    if ! flashrom -p linux_spi:dev="$BIOS_SPIDEV" -r "${tempfile}" -c "$BIOS_CHIP"; then
      echo "Flashrom failed. Retrying without -c"
      do_retry "read" "${tempfile}"
    fi

    # Make sure data is fully read
    datasize=$(stat -c%s "${tempfile}")
    if [ "${datasize}" -lt "${FLASH_SIZE}" ]; then
       echo "Unable to store all the data. Will not erase SPI"
       rm "${tempfile}"
       exit 1
    fi 
    
    # saving header, and pdr.
    # conv option not available in bmc busybox dd (can't do conv=nosync to prevent file from being truncated), so saving header as well.
    # we are backing from 00000000 to 00020fff which is 18 bits total. so we will use 9 bits for bs and 9 bits for count to get to the needed regions.
    if ! /bin/dd if="${tempfile}" of="${header_pdr_file}" bs=512 count=512; then
      echo "Running dd in backup_image failed"
      exit 1
    fi
    
    # The full image is no longer needed, so removed it
    rm "${tempfile}"

}

do_write() {
    bios_image="$1"
    if [ -n "$(aboot_version "$bios_image")" ]; then
        create_aboot_conf "$DEFAULT_ABOOT_CONF" || exit 1
        create_bios_image "$ABOOT_CONF_START" "$ABOOT_CONF_SIZE" \
                          "$bios_image" "$TEMP_ABOOT_CONF" || exit 1
        bios_image="$TEMP_BIOS_IMAGE"
    fi

    if [ ! -e /mnt/data/header_pdr.data ]; then
      backup_image
    fi
    echo " writing header and payload ... "
    if ! flashrom --layout /etc/yamp_bios.layout --image header --image payload -p linux_spi:dev="$BIOS_SPIDEV" -w "$bios_image" -c "$BIOS_CHIP"; then
      echo "flashrom failed. Retrying without -c"
      do_retry "write" "$bios_image"
    fi
}

do_recover() {
  echo "Recovering PDR region"
  if [ ! -e /mnt/data/header_pdr.data ]; then
    echo "recovery data /mnt/data/header_pdr.data doesn't exist. Exiting ..."
    exit 1
  fi

  # Getting full image ready. Image must be the same as original size
  if ! file=$(mktemp); then
    echo "Running mktemp in do_recover failed"
    exit 1
  fi
  cp /mnt/data/header_pdr.data "${file}"

  # So we have 16515072 bytes left which we will pad with zero so that we can get to the flashsize which is 16777216 bytes
  # Therefore, we will use a bs of 512 with a 32256 as count
  if ! /bin/dd if=/dev/zero bs=512 count=32256 >> "${file}"; then
    echo "Running dd in do_recover failed"
    exit 1
  fi

  #Recover pdr region which includes the idprom region
  if ! flashrom --layout /etc/yamp_bios.layout --image header --image pdr -p linux_spi:dev="$BIOS_SPIDEV" -w "${file}" -c "$BIOS_CHIP"; then
    echo "flashrom failed. Retrying without the -c"
    do_retry "write" "${file}"
  fi
  rm "${file}"
}

probe_chips() {
  if flashrom -p linux_spi:dev="$BIOS_SPIDEV" | grep "N25Q128" > /dev/null 2>&1; then
    BIOS_CHIP="N25Q128..3E"
  fi
}

connect_spi

probe_chips

if [ "$1" == "erase" ]; then
    do_erase
elif [ "$1" == "read" ]; then
    do_read "$2"
elif [ "$1" == "write" ]; then
    do_write "$2"
elif [ "${1}" == "recover" ]; then
    do_recover
else
    usage
fi

