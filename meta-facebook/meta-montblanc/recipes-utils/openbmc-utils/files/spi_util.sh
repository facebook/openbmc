#!/bin/sh
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#
#shellcheck disable=SC1091
#shellcheck disable=SC2086
. /usr/local/bin/openbmc-utils.sh

ret=0
trap cleanup_spi INT TERM QUIT EXIT

PIDFILE="/var/run/spi_util.pid"
check_duplicate_process()
{
    exec 2>$PIDFILE
    flock -n 2 || (echo "Another process is running" && exit 1)
    ret=$?
    if [ $ret -eq 1 ]; then
      exit 1
    fi
    pid=$$
    echo $pid 1>&200
}

remove_pid_file()
{
    if [ -f $PIDFILE ]; then
        rm $PIDFILE
    fi
}

SPI1_DRIVER="1e630000.spi"
ASPEED_SMC_PATH="/sys/bus/platform/drivers/spi-aspeed-smc/"

mtd_driver_unbind_spi1() {
    [ -e ${ASPEED_SMC_PATH}${SPI1_DRIVER} ] && echo ${SPI1_DRIVER} > ${ASPEED_SMC_PATH}unbind
}

mtd_driver_rebind_spi1() {
    # Sometime kernel may complain "vmap allocation for size 268439552 failed: use vmalloc=<size> to increase size"
    # until now the root cause is not clear, still need more investigation
    mtd_driver_unbind_spi1
    echo ${SPI1_DRIVER} > ${ASPEED_SMC_PATH}bind

    # add more delay to make sure the driver has probe done. 
    sleep 1;

    SPI1_MTD_PATH=$(realpath /run/devmap/spi-devices/spi1-flash)
    if [ ! -e "$SPI1_MTD_PATH" ]; then
        echo "cannot find SPI1 MTD path [$SPI1_MTD_PATH]"
        exit 1
    fi
    SPI1_MTD_INDEX=$(echo "$SPI1_MTD_PATH" | sed 's_/dev/mtd__g')
}

read_spi1_dev(){
    dev=$1
    file=$2
    echo "Reading ${dev} to $file..."
    flashrom -p linux_mtd:dev=$SPI1_MTD_INDEX -r "$file"
}

write_spi1_dev(){
    dev=$1
    file=$2
    echo "Writing $file to ${dev}..."

    tmpfile="/tmp/temp_image"
    flashsize=$(flashrom -p linux_mtd:dev=$SPI1_MTD_INDEX | grep -i kB | xargs echo | cut -d '(' -f 2 | cut -d ' ' -f 0)
    targetsize=$((flashsize * 1024))

    case ${dev} in
        "COME_BIOS"|"PROT"|"IOB_FPGA")
        # store to the temp-file
        cp "$file" "$tmpfile"
        ;;        
    esac

    #extend the temp-file
    filesize=$(stat -c%s $tmpfile)
    echo $filesize
    add_size=$((targetsize - filesize))
    echo $add_size
    dd if=/dev/zero bs="$add_size" count=1 | tr "\000" "\377" >> "$tmpfile"

    echo flashrom -p linux_mtd:dev=$SPI1_MTD_INDEX -w "$tmpfile"

    flashrom -p linux_mtd:dev=$SPI1_MTD_INDEX -w "$tmpfile"

    case ${dev} in
        "IOB_FPGA")
            # TODO: add reset signal component after programed
        ;;
        "COME_BIOS")
            # no need action, let user run command to reset COMe [wedge_power.sh reset]
        ;;
        "PROT")
            # TODO: need confirm, this path use only for debuging
        ;;
    esac
}

erase_spi1_dev(){
    flashrom -p linux_mtd:dev=$SPI1_MTD_INDEX -E
}

config_spi1_pin_and_path(){
    dev=$1

    # enable i2c buff BMC able to access 
    #  i2c-0  for COME CPLD  (0x1f)
    #  i2c-1  for SCM CPLD   (0x35)
    gpiocli -s BMC_I2C1_EN set-value 1
    gpiocli -s BMC_I2C2_EN set-value 1
    case ${dev} in
        "IOB_FPGA")
            # SCM CPLD: BMC_SPI1 
            #                    to (X) IOB_FPGA   0
            #                       ( ) COME_BIOS  1
            #                       ( ) PRoT       2
            i2cset -f -y 1 0x35 0x34 0x0

            # BMC GPIO: IOB_FLASH 
            #                    to ( ) IOB_FPGA 0
            #                       (X) BMC_SPI1 1
            gpiocli -s IOB_FLASH_SEL set-value 1
        ;;
        "COME_BIOS")
            # SCM CPLD: BMC_SPI1 
            #                    to ( ) IOB_FPGA   0
            #                       (X) COME_BIOS  1
            #                       ( ) PRoT       2
            i2cset -f -y 1 0x35 0x34 0x1
            
            # BMC GPIO: BIOS_FLASH 
            #                    to ( ) NetLake CPU 0
            #                       (X) BMC_SPI1    1
            gpiocli -s SPI_MUX_SEL set-value 1
            
            # COME CPLD: BIOS_FLASH_CS0
            #                    to ( ) BMC_SPI1_CS1 0x7
            #                       (X) BMC_SPI1_CS0 0x5
            #                       ( ) SPI_PCH_CS0 0x4
            #                       ( ) SPI_PCH_CS1 0x6
            i2cset -f -y 0 0x1f 0xa 0x5
        ;;
        "PROT")
            # TODO: need confirm, this path use only for debuging
            # SCM CPLD: set BMC_SPI1 
            #                    to ( ) IOB_FPGA   0
            #                       (X) COME_BIOS  1
            #                       ( ) PRoT       2
            i2cset -f -y 1 0x35 0x34 0x2
        ;;
    esac
    mtd_driver_rebind_spi1
}

cleanup_spi(){
    if [ $ret -eq 1 ]; then
        exit 1
    fi

    # select muxing FLASH to normal mode
    # BMC GPIO: IOB_FLASH 
    #                    to (X) IOB_FPGA    0
    #                       ( ) BMC_SPI1    1
    gpiocli -s IOB_FLASH_SEL set-value 0
    # BMC GPIO: BIOS_FLASH 
    #                    to (X) NetLake CPU 0
    #                       ( ) BMC_SPI1    1
    gpiocli -s SPI_MUX_SEL set-value 0

    rm -rf /tmp/*_spi*_tmp
    remove_pid_file
}

operate_spi1_dev(){
    op=$1
    dev=$2
    file=$3
    ## Operate devices ##
    case ${op} in
        "read")
                read_spi1_dev "$dev" "$file"
        ;;
        "write")
                if [ ! -f "$file" ]; then
                    echo "$file not exist!!!"
                    exit 1
                fi
                write_spi1_dev "$dev" "$file"
        ;;
        "erase")
                erase_spi1_dev "$dev"
        ;;
        *)
            echo "Operation $op is not defined."
        ;;
    esac
    mtd_driver_unbind_spi1
}

ui(){
    . /usr/local/bin/openbmc-utils.sh
    op=$1
    spi=$2

    ## Open the path to device ##
    case ${spi} in
        "spi1")
            dev=$3
            file=$4
            config_spi1_pin_and_path "$dev"
            operate_spi1_dev "$op" "$dev" "$file"
        ;;
        *)
            echo "No such SPI bus."
            return 1
        ;;
    esac
}

usage(){
    program=$(basename "$0")
    echo "Usage:"
    echo "  $program <op> <spi> <spi device> <file>"
    echo "    <op>          : read, write, erase"
    echo "    <spi>         : spi1"
    echo "    <spi1 device> : IOB_FPGA, COME_BIOS"
    echo ""
    echo "Examples:"
    echo "  $program write spi1 IOB_FPGA iofpga.bit"
    echo ""
}

check_parameter(){
    program=$(basename "$0")
    op=$1
    spi=$2

    # check operations.
    case ${op} in
        "read" | "write" | "erase")
            ;;
        *)
            return 1
            ;;
    esac

    case ${spi} in
        # check spi 1 device and parameters.
        "spi1")
            dev=$3
            file=$4
            case ${dev} in
                "IOB_FPGA"|"COME_BIOS")
                    case ${op} in
                        "read" | "write")
                            if [ $# -ne 4 ]; then
                                return 1
                            fi
                            ;;
                        "erase")
                            if [ $# -ne 3 ]; then
                                return 1
                            fi
                            ;;
                    esac
                    ;;
                "PROT")
                    echo "PRoT upgrade not support yet"
                    return 1
                    ;;
                *)
                    return 1
                    ;;
            esac
            ;;

        # check device and parameters fail.
        *)
            return 1
            ;;
    esac

    return 0
}


# check_duplicate_process - to check if there is the duplicate spi util process, will return fail and stop execute command.
check_duplicate_process

# check_parameter - to check command parameters
check_parameter "$@"
is_par_ok=$?

if [ $is_par_ok -eq 0 ]; then
    ui "$@"
    exit 0
else
    usage
    exit 1
fi