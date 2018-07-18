#!/bin/sh

source /usr/local/bin/openbmc-utils.sh

trap disconnect_spi INT TERM QUIT EXIT

usage() {
    program=`basename "$0"`
    echo "Usage:"
    echo "$program <OP> <bios file>"
    echo "      <OP> : read, write, erase"
    exit -1
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
    # So we need to manually recover IDPROM after erase.
    #
    # 1. Backup IDPROM data before erasing
    echo "Reading IDPROM"
    tempfile="/tmp/idprom.data"
    flashrom -p linux_spi:dev=/dev/spidev2.0 -r $tempfile -c "MX25L12835F/MX25L12845E/MX25L12865E"

    # 2. Make sure data is fully read (otherwise, flashrom will complain)
    datasize=$(stat -c%s $tempfile)
    if [ $datasize -lt 16777216 ]; then
       echo "Unable to store IDPROM data. Will not erase SPI"
       rm $tempfile
       exit -1
    fi 

    # 3. Do the erase
    echo "Erasing the flash"
    flashrom -p linux_spi:dev=/dev/spidev2.0 -E -c "MX25L12835F/MX25L12845E/MX25L12865E"

    # 4. Write back IDPROM
    echo "Writing IDPROM back"
    flashrom --layout /etc/yamp_bios.layout --image idprom -p linux_spi:dev=/dev/spidev2.0 -w $tempfile -c "MX25L12835F/MX25L12845E/MX25L12865E"
    rm $tempfile
}

do_read() {
    flashrom -p linux_spi:dev=/dev/spidev2.0 -r "$1" -c "MX25L12835F/MX25L12845E/MX25L12865E"
}

do_write() {
    flashrom --layout /etc/yamp_bios.layout --image header --image payload -p linux_spi:dev=/dev/spidev2.0 -w "$1" -c "MX25L12835F/MX25L12845E/MX25L12865E"
}

connect_spi

if [ "$1" == "erase" ]; then
    do_erase
elif [ "$1" == "read" ]; then
        do_read "$2"
elif [ "$1" == "write" ]; then
        do_write "$2"
else
    usage
fi

