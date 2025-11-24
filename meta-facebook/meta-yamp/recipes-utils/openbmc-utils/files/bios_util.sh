#!/bin/sh

# shellcheck disable=SC1091
# shellcheck disable=SC2012
# shellcheck disable=SC2039
# shellcheck disable=SC2086
. /usr/local/bin/openbmc-utils.sh

trap cleanup INT TERM QUIT EXIT

# Temp files for storing bios file.
TEMP_BIOS_IMAGE="/tmp/tmp_bios_image"
# Temp file for storing aboot_conf data
TEMP_ABOOT_CONF="/tmp/aboot_conf.bin"
DEFAULT_ABOOT_CONF="$BMC_CONF_FILE"

BIOS_SPIDEV="/dev/spidev2.0"
BIOS_CHIP="MX25L12835F/MX25L12845E/MX25L12865E"

popts=""
init_aconf=0

cleanup() {
    disconnect_spi
    rm -f $TEMP_ABOOT_CONF $TEMP_BIOS_IMAGE
}

usage() {
    program=$(basename "$0")
    echo "Usage:"
    echo "$program <OP> <bios file> [--partition <partition>]"
    echo "      <OP> : read, write, erase, recover"
    echo "      [<partition>] : partition of layout file; defaults to total (all sections)"
    echo "                      specify image for Aboot image sections"
    echo "$program write <bios file> [--init-aconf]"
    echo "      If --init-aconf is specified, the aboot_conf section will be programmed"
    echo "      after writing the bios file."
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
    if ! flashrom $popts -p linux_spi:dev="$BIOS_SPIDEV" -w "$2"; then
      echo "flashrom without -c failed"
    fi
  elif [ "$1" = "erase" ]; then
    if ! flashrom -p linux_spi:dev="$BIOS_SPIDEV" -E; then
      echo "flashrom without -c option failed as well"
    fi
  else # reading
    if ! flashrom $popts -p linux_spi:dev="$BIOS_SPIDEV" -r "$2"; then
      echo "flashrom without -c option failed"
    fi
  fi
}

aboot_version() {
    echo "Reading aboot version..."
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
    if ! flashrom $popts -p linux_spi:dev="$BIOS_SPIDEV" -r "$1" -c "$BIOS_CHIP"; then
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
      if [ $init_aconf -eq 1 ]; then
        create_aboot_conf_image "$DEFAULT_ABOOT_CONF" "$bios_image" "$TEMP_BIOS_IMAGE" || exit 1
        bios_image="$TEMP_BIOS_IMAGE"
      fi
    fi

    if [ ! -e /mnt/data/header_pdr.data ]; then
      backup_image
    fi
	 if [ -z "$popts" ]; then
		popts="-l $LAYOUT_FILE -i header -i payload"
    	echo " writing header and payload ... "
    else
		echo " writing partition(s) ... "
	 fi
    if ! flashrom $popts -p linux_spi:dev="$BIOS_SPIDEV" -w "$bios_image" -c "$BIOS_CHIP"; then
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
  popts="-l $LAYOUT_FILE -i header -i pdr"
  if ! flashrom $popts -p linux_spi:dev="$BIOS_SPIDEV" -w "${file}" -c "$BIOS_CHIP"; then
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

# Image partitions are non-contiguous so can't be specified by
# a single partition name.
IMAGE_PARTITIONS="normal microcode bootblock fallback"

get_partition_opts() {
  if [ "$1" = "--partition" ]; then
    if [ -z "$2" ]; then
      echo "Missing partition argument"
      usage
    fi
    partitions="$2"
  else
    return
  fi

  if [ "$partitions" = "image" ]; then
    partitions="$IMAGE_PARTITIONS"
  fi

  popts="-l $LAYOUT_FILE"
  for partition in $partitions; do
    popts="$popts -i $partition"
  done
}

if [ "$1" = "erase" ]; then
  do_erase
elif [ "$1" = "read" ]; then
  get_partition_opts "$3" "$4"
  do_read "$2"
elif [ "$1" = "write" ]; then
  image_arg="$2"
  while [ -n "$3" ]; do
    case $3 in
      --partition)
        get_partition_opts "$3" "$4"
        shift 2
        ;;
      --init-aconf)
        init_aconf=1
        shift
        ;;
      *)
        echo "Unknown argument $3"
        usage
        ;;
    esac
  done
  if [ -n "$popts" ] && [ $init_aconf -eq 1 ]; then
    echo "--init-aconf and --partition are mutually exclusive"
    usage
  fi
  do_write "$image_arg"
elif [ "$1" = "recover" ]; then
  do_recover
else
  usage
fi

