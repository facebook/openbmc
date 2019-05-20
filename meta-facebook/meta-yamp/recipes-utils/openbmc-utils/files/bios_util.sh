#!/bin/sh

source /usr/local/bin/openbmc-utils.sh

trap disconnect_spi INT TERM QUIT EXIT

usage() {
    program=`basename "$0"`
    echo "Usage:"
    echo "$program <OP> <bios file>"
    echo "      <OP> : read, write, erase, recover"
    exit 1
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
    # Layout is not supported in Erase. 
    # So we need to manually recover pdr (which include idprom) after erase.
    if [ ! -e /mnt/data/header_pdr.data ]; then
      backup_image
    fi

    # Do the erase
    echo "Erasing the flash"
    flashrom -p linux_spi:dev=/dev/spidev2.0 -E -c "MX25L12835F/MX25L12845E/MX25L12865E"
    
    # Recover header and pdr (which includes the idprom)
    do_recover
}

do_read() {
    flashrom -p linux_spi:dev=/dev/spidev2.0 -r "$1" -c "MX25L12835F/MX25L12845E/MX25L12865E"
}

backup_image(){
    echo "Backing up pdr"
    FLASH_SIZE=16777216  

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

    if ! flashrom -p linux_spi:dev=/dev/spidev2.0 -r "${tempfile}" -c "MX25L12835F/MX25L12845E/MX25L12865E"; then
      echo "Running flashrom in backup_image failed"
      exit 1
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
    if [ ! -e /mnt/data/header_pdr.data ]; then
      backup_image
    fi
    echo " writing header and payload ... "
    flashrom --layout /etc/yamp_bios.layout --image header --image payload -p linux_spi:dev=/dev/spidev2.0 -w "$1" -c "MX25L12835F/MX25L12845E/MX25L12865E"
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
  if ! flashrom --layout /etc/yamp_bios.layout --image header --image pdr -p linux_spi:dev=/dev/spidev2.0 -w "${file}" -c "MX25L12835F/MX25L12845E/MX25L12865E"; then
    echo "Running flashrom in do_recover failed"
    exit 1
  fi
  rm "${file}"
}

connect_spi

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

