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
# shellcheck source=/dev/null
. /usr/local/bin/openbmc-utils.sh

SPI1_MTD_INDEX=7
SPI1_PATH="/sys/bus/platform/drivers/spi-aspeed-smc/"
SPI1_DRIVER="1e630000.spi"
IOB_FPGA_PRG_INIT="${SCMCPLD_SYSFS_DIR}/iob_fpga_program"
PIDFILE="/var/run/spi_util.pid"

export_gpio()
{
    setup_gpio SPI_MUX_SEL        GPIOQ6 out 0
    setup_gpio BMC_IOB_FLASH_SEL  GPIOG4 out 0
    setup_gpio BMC_I2C1_EN        GPIOG0 out 0
    sleep 1
}

unexport_gpio()
{
    gpiocli --shadow SPI_MUX_SEL       unexport
    gpiocli --shadow BMC_IOB_FLASH_SEL unexport
    gpiocli --shadow BMC_I2C1_EN       unexport
}


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
    rm -f "$PIDFILE"
}

mtd_driver_unbind_spi() {
    SPI_PATH=$1
    SPI_DRIVER=$2
    [ -e "${SPI_PATH}" ] && echo "${SPI_DRIVER}" > "${SPI_PATH}"/unbind
}

mtd_driver_rebind_spi() {
    SPI_PATH=$1
    SPI_DRIVER=$2

    # Sometime kernel may complain "vmap allocation for size 268439552 failed: use vmalloc=<size> to increase size"
    # until now the root cause is not clear, still need more investigation
    mtd_driver_unbind_spi "$SPI_PATH" "$SPI_DRIVER"
    echo "${SPI_DRIVER}" > "${SPI_PATH}"/bind

    # add more delay to make sure the driver has probe done.
    sleep 1;
}

read_spi1_dev(){
    dev=$1
    file=$2
    echo "Reading ${dev} to $file..."
    flashrom -p linux_mtd:dev="$SPI1_MTD_INDEX" -r "$file"
}

write_spi1_dev(){
    dev=$1
    file=$2
    echo "Writing $file to ${dev}..."

    tmpfile="/tmp/temp_image"
    flashsize=$(flashrom -p linux_mtd:dev="$SPI1_MTD_INDEX" | grep -i kB | xargs echo | cut -d '(' -f 2 | cut -d ' ' -f 0)
    targetsize=$((flashsize * 1024))

    # store to the temp-file
    cp "$file" "$tmpfile"

    #extend the temp-file
    filesize=$(stat -c%s $tmpfile)
    add_size=$((targetsize - filesize))
    dd if=/dev/zero bs="$add_size" count=1 | tr "\000" "\377" >> "$tmpfile"

    echo flashrom -p linux_mtd:dev="$SPI1_MTD_INDEX" -w "$tmpfile"

    flashrom -p linux_mtd:dev="$SPI1_MTD_INDEX" -w "$tmpfile"
    ret=$?
    if [ $((ret)) -eq 0 ]; then
        case ${dev} in
            "IOB_FPGA")
                # set the IOB FLASH back to IOB FPGA
                # BMC GPIO: IOB_FLASH 
                #                    to (X) IOB_FPGA 0
                #                       ( ) BMC_SPI1 1
                gpiocli -s BMC_IOB_FLASH_SEL set-value 0

                #Reinit the IOB_FPGA.  
                echo 0 > "$IOB_FPGA_PRG_INIT"
                sleep 1
                echo 1 > "$IOB_FPGA_PRG_INIT"
            ;;
            "COME_BIOS")
                # no need action, let user run command to reset COMe [wedge_power.sh reset]
            ;;
        esac
    fi
}

erase_spi1_dev(){
    flashrom -p linux_mtd:dev="$SPI1_MTD_INDEX" -E
}

config_spi1_pin_and_path(){
    dev=$1

    case ${dev} in
        "IOB_FPGA")
            # SCM CPLD: BMC_SPI1 
            #                    to (X) IOB_FPGA   0
            #                       ( ) COME_BIOS  1
            #                       ( ) PRoT       2
            echo 0x0 > "$SCMCPLD_SYSFS_DIR/spi_select" 

            # BMC GPIO: IOB_FLASH 
            #                    to ( ) IOB_FPGA 0
            #                       (X) BMC_SPI1 1
            gpiocli -s BMC_IOB_FLASH_SEL set-value 1
        ;;
        "COME_BIOS")
            # SCM CPLD: BMC_SPI1 
            #                    to ( ) IOB_FPGA   0
            #                       (X) COME_BIOS  1
            #                       ( ) PRoT       2
            echo 0x1 > "$SCMCPLD_SYSFS_DIR/spi_select" 
            
            # BMC GPIO: BIOS_FLASH 
            #                    to ( ) NetLake CPU 0
            #                       (X) BMC_SPI1    1
            gpiocli -s SPI_MUX_SEL set-value 1
            
            # COME CPLD: BIOS_FLASH_CS0
            #                    to ( ) BMC_SPI1_CS1 0x7
            #                       (X) BMC_SPI1_CS0 0x5
            #                       ( ) SPI_PCH_CS0 0x4
            #                       ( ) SPI_PCH_CS1 0x6
            gpiocli -s BMC_I2C1_EN set-value 1
            i2cset -f -y 0 0x1f 0xa 0x5
            gpiocli -s BMC_I2C1_EN set-value 0
        ;;
    esac
    sleep 1
    mtd_driver_rebind_spi "$SPI1_PATH" "$SPI1_DRIVER"
}

cleanup_spi(){
    # select muxing FLASH to normal mode
    # BMC GPIO: IOB_FLASH 
    #                    to (X) IOB_FPGA    0
    #                       ( ) BMC_SPI1    1
    gpiocli -s BMC_IOB_FLASH_SEL set-value 0
    # BMC GPIO: BIOS_FLASH 
    #                    to (X) NetLake CPU 0
    #                       ( ) BMC_SPI1    1
    gpiocli -s SPI_MUX_SEL set-value 0

    rm -rf /tmp/*_spi*_tmp
    unexport_gpio
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
    mtd_driver_unbind_spi "$SPI1_PATH"  "$SPI1_DRIVER"
}
