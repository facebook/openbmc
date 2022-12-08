#!/bin/sh
#
# Copyright 2020-present Facebook. All Rights Reserved.
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

SPI1_MTD_INDEX=7
SPI1_DRIVER="1e630000.spi"
if [ -e /sys/bus/platform/drivers/spi-aspeed-smc ]; then
    ASPEED_SMC_PATH="/sys/bus/platform/drivers/spi-aspeed-smc/"
else
    ASPEED_SMC_PATH="/sys/bus/platform/drivers/aspeed-smc/"
fi

mtd_driver_unbind_spi1() {
    [ -e ${ASPEED_SMC_PATH}${SPI1_DRIVER} ] && echo ${SPI1_DRIVER} > ${ASPEED_SMC_PATH}unbind
}

mtd_driver_rebind_spi1() {
    # Sometime kernel may complain "vmap allocation for size 268439552 failed: use vmalloc=<size> to increase size"
    # until now the root cause is not clear, still need more investigation
    mtd_driver_unbind_spi1
    echo ${SPI1_DRIVER} > ${ASPEED_SMC_PATH}bind
}

read_spi1_dev(){
    dev=$1
    file=$2
    echo "Reading ${dev} to $file..."
    flashrom -p linux_mtd:dev=$SPI1_MTD_INDEX -r "$file"
}

read_spi2_dev(){
    dev=$1
    file=$2
    file_temp="$file"_temp
    echo "Reading ${dev} to $file..."
    # Sometimes the read back BCM5387_EE image one bit fail.
    # still not found the real root cause
    for n in 1 2 3 4 5
    do
        echo "Reading $file $n times"
        dd if=/sys/devices/platform/spi-gpio/spi_master/spi2/spi2.2/spi2.20/nvmem of="$file"
        sleep 1
        dd if=/sys/devices/platform/spi-gpio/spi_master/spi2/spi2.2/spi2.20/nvmem of="$file_temp"
        if ! diff $file $file_temp >/dev/null; then
            # read-back file differ
            rm -rf "$file"*
        else
            # read-back file same
            rm -rf $file_temp;
            return;
        fi
    done
    logger -p user.err "Unable to read the correct $file after 5 retries"
    exit 1
}

write_spi1_dev(){
    dev=$1
    file=$2
    echo "Writing $file to ${dev}..."

    tmpfile="$file"_ext
    flashsize=$(flashrom -p linux_mtd:dev=$SPI1_MTD_INDEX | grep -i kB | xargs echo | cut -d '(' -f 2 | cut -d ' ' -f 0)
    targetsize=$((flashsize * 1024))

    case ${dev} in
        "PCIE_SW"|"TH4_PCIE_FLASH"|"COME_BIOS"|"DOM_FPGA_ALL"|"DOM_FPGA_PIM1"|"DOM_FPGA_PIM2"|"DOM_FPGA_PIM3"|"DOM_FPGA_PIM4"|"DOM_FPGA_PIM5"|"DOM_FPGA_PIM6"|"DOM_FPGA_PIM7"|"DOM_FPGA_PIM8")
        # store to the temp-file
        cp "$file" "$tmpfile"
        ;;

	"IOB_FPGA")
        # remove fpga header size 112 and then store to the temp-file
        dd if="$file" skip=1 bs=112 of="$tmpfile"
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
            echo 0 > /sys/bus/i2c/devices/12-003e/sys_cpld_fpga_rst_n
            sleep 1
            echo 1 > /sys/bus/i2c/devices/12-003e/sys_cpld_fpga_rst_n
        ;;

        "DOM_FPGA_PIM1")
            echo 0 > /sys/bus/i2c/devices/12-003e/pim1_smb_pg
            sleep 1
            echo 1 > /sys/bus/i2c/devices/12-003e/pim1_smb_pg
        ;;
        "DOM_FPGA_PIM2")
            echo 0 > /sys/bus/i2c/devices/12-003e/pim2_smb_pg
            sleep 1
            echo 1 > /sys/bus/i2c/devices/12-003e/pim2_smb_pg
        ;;
        "DOM_FPGA_PIM3")
            echo 0 > /sys/bus/i2c/devices/12-003e/pim3_smb_pg
            sleep 1
            echo 1 > /sys/bus/i2c/devices/12-003e/pim3_smb_pg
        ;;
        "DOM_FPGA_PIM4")
            echo 0 > /sys/bus/i2c/devices/12-003e/pim4_smb_pg
            sleep 1
            echo 1 > /sys/bus/i2c/devices/12-003e/pim4_smb_pg
        ;;
        "DOM_FPGA_PIM5")
            echo 0 > /sys/bus/i2c/devices/12-003e/pim5_smb_pg
            sleep 1
            echo 1 > /sys/bus/i2c/devices/12-003e/pim5_smb_pg
        ;;
        "DOM_FPGA_PIM6")
            echo 0 > /sys/bus/i2c/devices/12-003e/pim6_smb_pg
            sleep 1
            echo 1 > /sys/bus/i2c/devices/12-003e/pim6_smb_pg
        ;;
        "DOM_FPGA_PIM7")
            echo 0 > /sys/bus/i2c/devices/12-003e/pim7_smb_pg
            sleep 1
            echo 1 > /sys/bus/i2c/devices/12-003e/pim7_smb_pg
        ;;
        "DOM_FPGA_PIM8")
            echo 0 > /sys/bus/i2c/devices/12-003e/pim8_smb_pg
            sleep 1
            echo 1 > /sys/bus/i2c/devices/12-003e/pim8_smb_pg
        ;;
    esac
}

write_spi2_dev(){
    dev=$1
    file=$2
    echo "Writing $file to ${dev}..."
    dd if="$file" of=/sys/devices/platform/spi-gpio/spi_master/spi2/spi2.2/spi2.20/nvmem 
}

erase_spi1_dev(){
    flashrom -p linux_mtd:dev=$SPI1_MTD_INDEX -E
}

erase_spi2_dev(){
    echo 1 > /sys/devices/platform/spi-gpio/spi_master/spi2/spi2.2/erase
}

setup_pims_switch_mux(){

#
# This function is used to bypass the command through FPGA to PCA9534, to switch the mux from PIM FPGA to IOB FPGA that BMC can access PIM Flash through IOB FPGA via SPI protocol.
#
# The registers at address 0XF0 and above defined in this section is dedicated for BMC only.
# CPU can’t access 0XF0-0XFF registers in this section. BMC can use the address/data/command registers defined in address 0XF0- 0XFC to access FPGA full memory space.
# 0xFF is a debug register.
#
# 0xF0	RW	Indirect Access Address register [31:24]
# 0xF1	RW	Indirect Access Address register [23:16]
# 0xF2	RW	Indirect Access Address register [15:8]
# 0xF3	RW	Indirect Access Address register [7:0]
#
# 0xF4  RW      Indirect Access Write data register [31:24]
# 0xF5  RW      Indirect Access Write data [23:16]
# 0xF6  RW      Indirect Access Write data [15:8]
# 0xF7  RW      Indirect Access Write data [7:0]
#
# 0xFC	RW	Indirect Access Command Register.
# bit0 = 1 is for write; bit0 = 0 is for read.
# Writing to this register will trigger an indirect access.
#
# Set 0x0000148c to 0x00000003
#
    # Set Indirect Access Address 0x0000148c
    i2cset -y -f 13 0x35 0xf0 0x0
    i2cset -y -f 13 0x35 0xf1 0x0
    i2cset -y -f 13 0x35 0xf2 0x14
    i2cset -y -f 13 0x35 0xf3 0x8c
    # Set Write Data 0x00000003
    i2cset -y -f 13 0x35 0xf4 0x0
    i2cset -y -f 13 0x35 0xf5 0x0
    i2cset -y -f 13 0x35 0xf6 0x0
    i2cset -y -f 13 0x35 0xf7 0x3
    # Set Command Write Action
    i2cset -y -f 13 0x35 0xfc 0x1

#
# Set 0x00001484 to 0x80220200
#

    # Set Indirect Access Address 0x00001484
    i2cset -y -f 13 0x35 0xf0 0x0
    i2cset -y -f 13 0x35 0xf1 0x0
    i2cset -y -f 13 0x35 0xf2 0x14
    i2cset -y -f 13 0x35 0xf3 0x84
    # Set Write Data 0x80220200
    i2cset -y -f 13 0x35 0xf4 0x80
    i2cset -y -f 13 0x35 0xf5 0x22
    i2cset -y -f 13 0x35 0xf6 0x2
    i2cset -y -f 13 0x35 0xf7 0x0
    # Set Command Write Action
    i2cset -y -f 13 0x35 0xfc 0x1

#
# Set 0x00001500 to 0x0000fc03
#

    # Set Indirect Access Address 0x00001500
    i2cset -y -f 13 0x35 0xf0 0x0
    i2cset -y -f 13 0x35 0xf1 0x0
    i2cset -y -f 13 0x35 0xf2 0x15
    i2cset -y -f 13 0x35 0xf3 0x0
    # Set Write Data 0x0000fc03
    i2cset -y -f 13 0x35 0xf4 0x0
    i2cset -y -f 13 0x35 0xf5 0x0
    i2cset -y -f 13 0x35 0xf6 0xfc
    i2cset -y -f 13 0x35 0xf7 0x3
    # Set Command Write Action
    i2cset -y -f 13 0x35 0xfc 0x1

    sleep 1

#
# Set 0x0000148c to 0x00000003
#

    # Set Indirect Access Address 0x0000148c
    i2cset -y -f 13 0x35 0xf0 0x0
    i2cset -y -f 13 0x35 0xf1 0x0
    i2cset -y -f 13 0x35 0xf2 0x14
    i2cset -y -f 13 0x35 0xf3 0x8c
    # Set Write Data 0x00000003
    i2cset -y -f 13 0x35 0xf4 0x0
    i2cset -y -f 13 0x35 0xf5 0x0
    i2cset -y -f 13 0x35 0xf6 0x0
    i2cset -y -f 13 0x35 0xf7 0x3
    # Set Command Write Action
    i2cset -y -f 13 0x35 0xfc 0x1

#
# Set 0x00001484 to 0x80220200
#

    # Set Indirect Access Address 0x00001484
    i2cset -y -f 13 0x35 0xf0 0x0
    i2cset -y -f 13 0x35 0xf1 0x0
    i2cset -y -f 13 0x35 0xf2 0x14
    i2cset -y -f 13 0x35 0xf3 0x84
    # Set Write Data 0x80220200
    i2cset -y -f 13 0x35 0xf4 0x80
    i2cset -y -f 13 0x35 0xf5 0x22
    i2cset -y -f 13 0x35 0xf6 0x2
    i2cset -y -f 13 0x35 0xf7 0x0
    # Set Command Write Action
    i2cset -y -f 13 0x35 0xfc 0x1

#
# Set 0x00001500 to 0x0000d501
#

    # Set Indirect Access Address 0x00001500
    i2cset -y -f 13 0x35 0xf0 0x0
    i2cset -y -f 13 0x35 0xf1 0x0
    i2cset -y -f 13 0x35 0xf2 0x15
    i2cset -y -f 13 0x35 0xf3 0x0
    # Set Write Data 0x0000d501
    i2cset -y -f 13 0x35 0xf4 0x0
    i2cset -y -f 13 0x35 0xf5 0x0
    i2cset -y -f 13 0x35 0xf6 0xd5
    i2cset -y -f 13 0x35 0xf7 0x01
    # Set Command Write Action
    i2cset -y -f 13 0x35 0xfc 0x1

    sleep 1

#
# Set 0x0000148c to 0x00000003
#

    # Set Indirect Access Address 0x0000148c
    i2cset -y -f 13 0x35 0xf0 0x0
    i2cset -y -f 13 0x35 0xf1 0x0
    i2cset -y -f 13 0x35 0xf2 0x14
    i2cset -y -f 13 0x35 0xf3 0x8c
    # Set Write Data 0x00000003
    i2cset -y -f 13 0x35 0xf4 0x0
    i2cset -y -f 13 0x35 0xf5 0x0
    i2cset -y -f 13 0x35 0xf6 0x0
    i2cset -y -f 13 0x35 0xf7 0x3
    # Set Command Write Action
    i2cset -y -f 13 0x35 0xfc 0x1

#
# Set 0x00001484 to 0x80220200
#

    # Set Indirect Access Address 0x00001484
    i2cset -y -f 13 0x35 0xf0 0x0
    i2cset -y -f 13 0x35 0xf1 0x0
    i2cset -y -f 13 0x35 0xf2 0x14
    i2cset -y -f 13 0x35 0xf3 0x84
    # Set Write Data 0x80220200
    i2cset -y -f 13 0x35 0xf4 0x80
    i2cset -y -f 13 0x35 0xf5 0x22
    i2cset -y -f 13 0x35 0xf6 0x2
    i2cset -y -f 13 0x35 0xf7 0x0
    # Set Command Write Action
    i2cset -y -f 13 0x35 0xfc 0x1

#
# Set 0x00001500 to 0x0000fc03
#

    # Set Indirect Access Address 0x00001500
    i2cset -y -f 13 0x35 0xf0 0x0
    i2cset -y -f 13 0x35 0xf1 0x0
    i2cset -y -f 13 0x35 0xf2 0x15
    i2cset -y -f 13 0x35 0xf3 0x0
    # Set Write Data 0x0000fc03
    i2cset -y -f 13 0x35 0xf4 0x0
    i2cset -y -f 13 0x35 0xf5 0x0
    i2cset -y -f 13 0x35 0xf6 0xfc
    i2cset -y -f 13 0x35 0xf7 0x3
    # Set Command Write Action
    i2cset -y -f 13 0x35 0xfc 0x1

    sleep 1

#
# Set 0x0000148c to 0x00000003
#

    # Set Indirect Access Address 0x0000148c
    i2cset -y -f 13 0x35 0xf0 0x0
    i2cset -y -f 13 0x35 0xf1 0x0
    i2cset -y -f 13 0x35 0xf2 0x14
    i2cset -y -f 13 0x35 0xf3 0x8c
    # Set Write Data 0x00000003
    i2cset -y -f 13 0x35 0xf4 0x0
    i2cset -y -f 13 0x35 0xf5 0x0
    i2cset -y -f 13 0x35 0xf6 0x0
    i2cset -y -f 13 0x35 0xf7 0x3
    # Set Command Write Action
    i2cset -y -f 13 0x35 0xfc 0x1

#
# Set 0x00001484 to 0x80220200
#

    # Set Indirect Access Address 0x00001484
    i2cset -y -f 13 0x35 0xf0 0x0
    i2cset -y -f 13 0x35 0xf1 0x0
    i2cset -y -f 13 0x35 0xf2 0x14
    i2cset -y -f 13 0x35 0xf3 0x84
    # Set Write Data 0x80220200
    i2cset -y -f 13 0x35 0xf4 0x80
    i2cset -y -f 13 0x35 0xf5 0x22
    i2cset -y -f 13 0x35 0xf6 0x2
    i2cset -y -f 13 0x35 0xf7 0x0
    # Set Command Write Action
    i2cset -y -f 13 0x35 0xfc 0x1

#
# Set 0x00001500 to 0x0000d501
#

    # Set Indirect Access Address 0x00001500
    i2cset -y -f 13 0x35 0xf0 0x0
    i2cset -y -f 13 0x35 0xf1 0x0
    i2cset -y -f 13 0x35 0xf2 0x15
    i2cset -y -f 13 0x35 0xf3 0x0
    # Set Write Data 0x0000d501
    i2cset -y -f 13 0x35 0xf4 0x0
    i2cset -y -f 13 0x35 0xf5 0x0
    i2cset -y -f 13 0x35 0xf6 0xd5
    i2cset -y -f 13 0x35 0xf7 0x01
    # Set Command Write Action
    i2cset -y -f 13 0x35 0xfc 0x1

}

config_spi1_pin_and_path(){
    dev=$1

    case ${dev} in
        "TH4_PCIE_FLASH")
            echo "[Setup] TH4_PCIE_FLASH"
            echo 0x00 > /sys/bus/i2c/devices/12-003e/spi1_sel
            #
            # FPGA 0x0074, [31:0] is 4 bytes address offset from 0x74 to 0x77 offset
            #
            # Set FPGA register offset 74h bit[15] SPI master mode to 1 - enable FPGA internal SPI masters
            #
            # Bit0 is for PIM#1; bit7 is for PIM#8.
            # Bit8 is for the SPI Flash attached on IOB FPGA. For testing
            #
            # Set FPGA register offset 74h bit[8:0] SPI selection bitmap to 0 - select none
            i2cset -y -f 13 0x35 0x76 0x80 b
            i2cset -y -f 13 0x35 0x77 0x00 b
        ;;
        "IOB_FPGA")
            echo "[Setup] IOB_FPGA"
            echo 0x01 > /sys/bus/i2c/devices/12-003e/spi1_sel
            #
            # FPGA 0x0074, [31:0] is 4 bytes address offset from 0x74 to 0x77 offset
            #
            # Set FPGA register offset 74h bit[15] SPI master mode to 1 - enable FPGA internal SPI masters
            #
            # Bit0 is for PIM#1; bit7 is for PIM#8.
            # Bit8 is for the SPI Flash attached on IOB FPGA. For testing
            #
            # Set FPGA register offset 74h bit[8:0] SPI selection bitmap to 0 - select none
            i2cset -y -f 13 0x35 0x76 0x80 b
            i2cset -y -f 13 0x35 0x77 0x00 b
        ;;
        "PCIE_SW")
            echo "[Setup] PCIE_SW"
            echo 0x02 > /sys/bus/i2c/devices/12-003e/spi1_sel
            #
            # FPGA 0x0074, [31:0] is 4 bytes address offset from 0x74 to 0x77 offset
            #
            # Set FPGA register offset 74h bit[15] SPI master mode to 1 - enable FPGA internal SPI masters
            #
            # Bit0 is for PIM#1; bit7 is for PIM#8.
            # Bit8 is for the SPI Flash attached on IOB FPGA. For testing
            #
            # Set FPGA register offset 74h bit[8:0] SPI selection bitmap to 0 - select none
            i2cset -y -f 13 0x35 0x76 0x80 b
            i2cset -y -f 13 0x35 0x77 0x00 b
        ;;
        "COME_BIOS")
            echo "[Setup] COME_BIOS"
            echo 0x03 > /sys/bus/i2c/devices/12-003e/spi1_sel
            #
            # FPGA 0x0074, [31:0] is 4 bytes address offset from 0x74 to 0x77 offset
            #
            # Set FPGA register offset 74h bit[15] SPI master mode to 1 - enable FPGA internal SPI masters
            #
            # Bit0 is for PIM#1; bit7 is for PIM#8.
            # Bit8 is for the SPI Flash attached on IOB FPGA. For testing
            #
            # Set FPGA register offset 74h bit[8:0] SPI selection bitmap to 0 - select none
            i2cset -y -f 13 0x35 0x76 0x80 b
            i2cset -y -f 13 0x35 0x77 0x00 b
        ;;
        "DOM_FPGA_ALL")
            echo "[Setup] DOM_FPGA_ALL"
            echo 0x06 > /sys/bus/i2c/devices/12-003e/spi1_sel
            #
            # FPGA 0x0074, [31:0] is 4 bytes address offset from 0x74 to 0x77 offset
            #
            # Set FPGA register offset 74h bit[15] SPI master mode to 0 - SPI legacy mode, SPI master is BMC.
            #
            # Bit0 is for PIM#1; bit7 is for PIM#8.
            # Bit8 is for the SPI Flash attached on IOB FPGA. For testing
            #
            # Set FPGA register offset 74h bit[8:0] SPI selection bitmap to 0x100 - Bit8 is for the SPI Flash attached on IOB FPGA. For testing
            i2cset -y -f 13 0x35 0x76 0x01 b
            i2cset -y -f 13 0x35 0x77 0x00 b
        ;;
        "DOM_FPGA_PIM1")
            echo "[Setup] DOM_FPGA_PIM1"
            echo 0x06 > /sys/bus/i2c/devices/12-003e/spi1_sel
            #
            # FPGA 0x0074, [31:0] is 4 bytes address offset from 0x74 to 0x77 offset
            #
            # Set FPGA register offset 74h bit[15] SPI master mode to 0 - SPI legacy mode, SPI master is BMC.
            #
            # Bit0 is for PIM#1; bit7 is for PIM#8.
            # Bit8 is for the SPI Flash attached on IOB FPGA. For testing
            #
            # Set FPGA register offset 74h bit[8:0] SPI selection bitmap to 0x01 - Bit0 is for PIM#1
            i2cset -y -f 13 0x35 0x76 0x00 b
            i2cset -y -f 13 0x35 0x77 0x01 b
            setup_pims_switch_mux
        ;;
        "DOM_FPGA_PIM2")
            echo "[Setup] DOM_FPGA_PIM2"
            echo 0x06 > /sys/bus/i2c/devices/12-003e/spi1_sel
            #
            # FPGA 0x0074, [31:0] is 4 bytes address offset from 0x74 to 0x77 offset
            #
            # Set FPGA register offset 74h bit[15] SPI master mode to 0 - SPI legacy mode, SPI master is BMC.
            #
            # Bit0 is for PIM#1; bit7 is for PIM#8.
            # Bit8 is for the SPI Flash attached on IOB FPGA. For testing
            #
            # Set FPGA register offset 74h bit[8:0] SPI selection bitmap to 0x02 - Bit1 is for PIM#2
            i2cset -y -f 13 0x35 0x76 0x10 b
            i2cset -y -f 13 0x35 0x77 0x02 b
            setup_pims_switch_mux
        ;;
        "DOM_FPGA_PIM3")
            echo "[Setup] DOM_FPGA_PIM3"
            echo 0x06 > /sys/bus/i2c/devices/12-003e/spi1_sel
            #
            # FPGA 0x0074, [31:0] is 4 bytes address offset from 0x74 to 0x77 offset
            #
            # Set FPGA register offset 74h bit[15] SPI master mode to 0 - SPI legacy mode, SPI master is BMC.
            #
            # Bit0 is for PIM#1; bit7 is for PIM#8.
            # Bit8 is for the SPI Flash attached on IOB FPGA. For testing
            #
            # Set FPGA register offset 74h bit[8:0] SPI selection bitmap to 0x04 - Bit2 is for PIM#3
            i2cset -y -f 13 0x35 0x76 0x20 b
            i2cset -y -f 13 0x35 0x77 0x04 b
            setup_pims_switch_mux
        ;;
        "DOM_FPGA_PIM4")
            echo "[Setup] DOM_FPGA_PIM4"
            echo 0x06 > /sys/bus/i2c/devices/12-003e/spi1_sel
            #
            # FPGA 0x0074, [31:0] is 4 bytes address offset from 0x74 to 0x77 offset
            #
            # Set FPGA register offset 74h bit[15] SPI master mode to 0 - SPI legacy mode, SPI master is BMC.
            #
            # Bit0 is for PIM#1; bit7 is for PIM#8.
            # Bit8 is for the SPI Flash attached on IOB FPGA. For testing
            #
            # Set FPGA register offset 74h bit[8:0] SPI selection bitmap to 0x08 - Bit3 is for PIM#4
            i2cset -y -f 13 0x35 0x76 0x30 b
            i2cset -y -f 13 0x35 0x77 0x08 b
            setup_pims_switch_mux
        ;;
        "DOM_FPGA_PIM5")
            echo "[Setup] DOM_FPGA_PIM5"
            echo 0x06 > /sys/bus/i2c/devices/12-003e/spi1_sel
            #
            # FPGA 0x0074, [31:0] is 4 bytes address offset from 0x74 to 0x77 offset
            #
            # Set FPGA register offset 74h bit[15] SPI master mode to 0 - SPI legacy mode, SPI master is BMC.
            #
            # Bit0 is for PIM#1; bit7 is for PIM#8.
            # Bit8 is for the SPI Flash attached on IOB FPGA. For testing
            #
            # Set FPGA register offset 74h bit[8:0] SPI selection bitmap to 0x10 - Bit4 is for PIM#5
            i2cset -y -f 13 0x35 0x76 0x40 b
            i2cset -y -f 13 0x35 0x77 0x10 b
            setup_pims_switch_mux
        ;;
        "DOM_FPGA_PIM6")
            echo "[Setup] DOM_FPGA_PIM6"
            echo 0x06 > /sys/bus/i2c/devices/12-003e/spi1_sel
            #
            # FPGA 0x0074, [31:0] is 4 bytes address offset from 0x74 to 0x77 offset
            #
            # Set FPGA register offset 74h bit[15] SPI master mode to 0 - SPI legacy mode, SPI master is BMC.
            #
            # Bit0 is for PIM#1; bit7 is for PIM#8.
            # Bit8 is for the SPI Flash attached on IOB FPGA. For testing
            #
            # Set FPGA register offset 74h bit[8:0] SPI selection bitmap to 0x20 - Bit5 is for PIM#6
            i2cset -y -f 13 0x35 0x76 0x50 b
            i2cset -y -f 13 0x35 0x77 0x20 b
            setup_pims_switch_mux
        ;;
        "DOM_FPGA_PIM7")
            echo "[Setup] DOM_FPGA_PIM7"
            echo 0x06 > /sys/bus/i2c/devices/12-003e/spi1_sel
            #
            # FPGA 0x0074, [31:0] is 4 bytes address offset from 0x74 to 0x77 offset
            #
            # Set FPGA register offset 74h bit[15] SPI master mode to 0 - SPI legacy mode, SPI master is BMC.
            #
            # Bit0 is for PIM#1; bit7 is for PIM#8.
            # Bit8 is for the SPI Flash attached on IOB FPGA. For testing
            #
            # Set FPGA register offset 74h bit[8:0] SPI selection bitmap to 0x40 - Bit6 is for PIM#7
            i2cset -y -f 13 0x35 0x76 0x60 b
            i2cset -y -f 13 0x35 0x77 0x40 b
            setup_pims_switch_mux
        ;;
        "DOM_FPGA_PIM8")
            echo "[Setup] DOM_FPGA_PIM8"
            echo 0x06 > /sys/bus/i2c/devices/12-003e/spi1_sel
            #
            # FPGA 0x0074, [31:0] is 4 bytes address offset from 0x74 to 0x77 offset
            #
            # Set FPGA register offset 74h bit[15] SPI master mode to 0 - SPI legacy mode, SPI master is BMC.
            #
            # Bit0 is for PIM#1; bit7 is for PIM#8.
            # Bit8 is for the SPI Flash attached on IOB FPGA. For testing
            #
            # Set FPGA register offset 74h bit[8:0] SPI selection bitmap to 0x80 - Bit7 is for PIM#8
            i2cset -y -f 13 0x35 0x76 0x70 b
            i2cset -y -f 13 0x35 0x77 0x80 b
            setup_pims_switch_mux
        ;;

        *)
            echo "Please enter {TH4_PCIE_FLASH, IOB_FPGA, PCIE_SW, COME_BIOS, DOM_FPGA_ALL, DOM_FPGA_PIM1, DOM_FPGA_PIM2, DOM_FPGA_PIM3, DOM_FPGA_PIM4, DOM_FPGA_PIM5, DOM_FPGA_PIM6, DOM_FPGA_PIM7, DOM_FPGA_PIM8}"
            exit 1
        ;;
    esac

    mtd_driver_rebind_spi1
    echo "Config SPI1 Done."
}

config_spi2_pin_and_path(){
    dev=$1

    case ${dev} in
        "BCM5387_EE")
            echo "[Setup] BCM5387_EE"
            echo 0x05 > /sys/bus/i2c/devices/12-003e/spi2_sel
        ;;
        "SMB_EE")
            echo "[Setup] SMB_EE"
            echo 0x04 > /sys/bus/i2c/devices/12-003e/spi2_sel
        ;;

        *)
            echo "Please enter {BCM5387_EE, SMB_EE}"
            exit 1
        ;;
    esac
    echo "Config SPI2 Done."
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

operate_spi2_dev(){
    op=$1
    dev=$2
    file=$3
    ## Operate devices ##
    case ${op} in
        "read")
                read_spi2_dev "$dev" "$file"
        ;;
        "write")
                if [ ! -f "$file" ]; then
                    echo "$file not exist!!!"
                    exit 1
                fi
                write_spi2_dev "$dev" "$file"
        ;;
        "erase")
                erase_spi2_dev "$dev"
        ;;
        *)
            echo "Operation $op is not defined."
        ;;
    esac
}


cleanup_spi(){
    if [ $ret -eq 1 ]; then
        exit 1
    fi
    echo 0 > "$SMBCPLD_SYSFS_DIR/spi_1_sel"

    rm -rf /tmp/*_spi*_tmp
    remove_pid_file
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
	"spi2")
            dev=$3
            file=$4
            config_spi2_pin_and_path "$dev"
            operate_spi2_dev "$op" "$dev" "$file"
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
    echo "  $program <op> <spi1|spi2> <spi device> <file>"
    echo "    <op>          : read, write, erase"
    echo "    <spi1 device> : TH4_PCIE_FLASH, IOB_FPGA, PCIE_SW, COME_BIOS, DOM_FPGA_ALL, DOM_FPGA_PIM1 ~ DOM_FPGA_PIM8"
    echo "    <spi2 device> : BCM5387_EE, SMB_EE"
    echo ""
    echo "Examples:"
    echo "  $program write spi1 DOM_FPGA_PIM7 domfpga.bit"
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
                "TH4_PCIE_FLASH"|"IOB_FPGA"|"PCIE_SW"|"COME_BIOS"|"DOM_FPGA_ALL"|"DOM_FPGA_PIM1"|"DOM_FPGA_PIM2"|"DOM_FPGA_PIM3"|"DOM_FPGA_PIM4"|"DOM_FPGA_PIM5"|"DOM_FPGA_PIM6"|"DOM_FPGA_PIM7"|"DOM_FPGA_PIM8")
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
                *)
                    return 1
                    ;;
            esac
            ;;
        # check spi 2 device and parameters.
        "spi2")
            dev=$3
            file=$4

            case ${dev} in
                "BCM5387_EE"|"SMB_EE")
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

