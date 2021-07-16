#!/bin/sh
#
# Copyright 2019-present Facebook. All Rights Reserved.
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

. /usr/local/bin/openbmc-utils.sh

output_reg=0x1
mode_reg=0x3
debug=0
m95m02_page_size=256
m95m02_page_count=1024
fpga_header_size=112

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

check_board_type()
{
    board_type=$(head -n 1 $SMBCPLD_SYSFS_DIR/board_type)
    ret_type=$?
    board_ver=$(head -n 1 $SMBCPLD_SYSFS_DIR/board_ver)
    ret_ver=$?
    if [ $ret_type -ne 0 ] || [ $ret_ver -ne 0 ]; then # get version error
        SWITCH_PCIE_FLASH="GB_PCIE_FLASH"
        MAC_TYPE="W400?"
    elif [ "$((board_type))" -eq 0 ]; then # Wedge400 any version
        SWITCH_PCIE_FLASH="TH3_PCIE_FLASH"
        MAC_TYPE="W400"
    elif [ "$((board_type))" -eq 1 ]; then # Wedge400-C any version
        SWITCH_PCIE_FLASH="GB_PCIE_FLASH"
        MAC_TYPE="W400C"
    else
        SWITCH_PCIE_FLASH="GB_PCIE_FLASH"
        MAC_TYPE="W400?"
    fi
}

check_flash_info()
{
    spi_no=$1
    flashrom -p linux_spi:dev=/dev/spidev${spi_no}.0
}

get_flash_first_type()
{
    ori_str=$(check_flash_info ${spi_no})
    type=$(echo ${ori_str} | cut -d '"' -f 2)
    if [ $type ];then
        echo $type
        return 0
    else
        echo "Get flash type error: [$ori_str]"
        exit 1
    fi
}

get_flash_size()
{
    spi_no=$1
    ori_str=$(check_flash_info ${spi_no})
    flash_sz=$(echo ${ori_str} | cut -d '(' -f 4 | cut -d ' ' -f 1)
    echo $flash_sz | grep -E -q '^[0-9]+$'
    num_ret=$?
    if [ $num_ret -eq 0 ];then
        echo $flash_sz
        return 0
    else
        echo "Get flash size error: [$ori_str]"
        return 1
    fi
}

# $1: input file size $2: flash size $3: output file path
pad_ff()
{
    out_file=$3
    pad_size=$(($2 - $1))
    dd if=/dev/zero bs=$pad_size count=1 | tr "\000" "\377" >> $out_file
}

resize_file()
{
    in_file=$1
    out_file=$2
    spi_no=$3
    storage_type=$4
    in_file_sz=$(stat -c%s $in_file)
    storage_sz=0
    if [ $storage_type ] && [ $storage_type = "m95m02" ];then
        storage_sz=$((m95m02_page_count * m95m02_page_size))
    else
        flash_sz=$(get_flash_size $spi_no)
        if [ $? -eq 0 ];then
            storage_sz=$((flash_sz * 1024))
        else
            echo "debug message: $flash_sz"
            exit 1
        fi
    fi

    if [ $in_file_sz -ne $storage_sz ];then
        cp $in_file $out_file
        pad_ff $in_file_sz $storage_sz $out_file
    else
        ln -s "$(realpath ${in_file})" ${out_file}
    fi
}

dump_gpio_config(){
    if [ $debug = 0 ]; then
        return 0
    fi
    echo "Dump SPI configurations..."
    if [ $1 = "SPI1" ];then
        openbmc_gpio_util.py dump | grep SPI1
    else
        openbmc_gpio_util.py dump | grep SPI2
    fi
}

export_spi1_pins(){
    gpiocli --shadow BMC_SPI1_CS0 -c aspeed-gpio -n GPIOI4 export > /dev/null 2>&1
    gpiocli --shadow BMC_SPI1_CLK -c aspeed-gpio -n GPIOI5 export > /dev/null 2>&1
    gpiocli --shadow BMC_SPI1_MOSI -c aspeed-gpio -n GPIOI6 export > /dev/null 2>&1
    gpiocli --shadow BMC_SPI1_MISO -c aspeed-gpio -n GPIOI7 export > /dev/null 2>&1
}

unexport_spi1_pins(){
    gpiocli --shadow BMC_SPI1_CS0 -c aspeed-gpio -n GPIOI4 unexport > /dev/null 2>&1
    gpiocli --shadow BMC_SPI1_CLK -c aspeed-gpio -n GPIOI5 unexport > /dev/null 2>&1
    gpiocli --shadow BMC_SPI1_MOSI -c aspeed-gpio -n GPIOI6 unexport > /dev/null 2>&1
    gpiocli --shadow BMC_SPI1_MISO -c aspeed-gpio -n GPIOI7 unexport > /dev/null 2>&1
}

# currently only EEPROM
set_spi1_to_gpio(){
    devmem_set_bit "$(scu_addr 7C)" 12
    devmem_set_bit "$(scu_addr 7C)" 13
    devmem_set_bit "$(scu_addr 80)" 15
    export_spi1_pins
    gpiocli --shadow BMC_SPI1_CLK set-direction out
    gpiocli --shadow BMC_SPI1_CS0 set-direction out
    gpiocli --shadow BMC_SPI1_MISO set-direction in
    gpiocli --shadow BMC_SPI1_MOSI set-direction out
    dump_gpio_config SPI1
}

set_spi1_to_spi(){
    devmem_set_bit "$(scu_addr 70)" 12
    unexport_spi1_pins
    dump_gpio_config SPI1
}

# currently no use
set_spi2_to_spi_cs(){
    cs=$1
    if [ ${cs} = 0 ]; then
        # spi2 cs0
        devmem_set_bit "$(scu_addr 88)" 26
        devmem_set_bit "$(scu_addr 88)" 27
        devmem_set_bit "$(scu_addr 88)" 28
        devmem_set_bit "$(scu_addr 88)" 29
    else
        # spi2 cs1
        devmem_set_bit "$(scu_addr 8C)" 0
        devmem_set_bit "$(scu_addr 88)" 27
        devmem_set_bit "$(scu_addr 88)" 28
        devmem_set_bit "$(scu_addr 88)" 29
    fi
    dump_gpio_config SPI2
}

# currently no use
set_spi2_to_gpio(){
    devmem_clear_bit "$(scu_addr 88)" 26
    devmem_clear_bit "$(scu_addr 88)" 27
    devmem_clear_bit "$(scu_addr 88)" 28
    devmem_clear_bit "$(scu_addr 88)" 29
    devmem_clear_bit "$(scu_addr 8C)" 0
    gpiocli --shadow BMC_SPI2_CLK set-direction out
    gpiocli --shadow BMC_SPI2_CS0 set-direction out
    gpiocli --shadow BMC_SPI2_CS1 set-direction out
    gpiocli --shadow BMC_SPI2_MISO set-direction in
    gpiocli --shadow BMC_SPI2_MOSI set-direction out
    dump_gpio_config SPI2
}

read_flash_to_file()
{
    spi_no=$1
    tmp_file=$2
    type=$(get_flash_first_type $spi_no)
    flashrom -p linux_spi:dev=/dev/spidev${spi_no}.0 -r $tmp_file -c $type
    if [ $? -ne 0 ];then
        echo "debug cmd: [flashrom -p linux_spi:dev=/dev/spidev${spi_no}.0 -r $tmp_file -c $type]"
        exit 1
    fi
}

write_flash_to_file(){
    spi_no=$1
    in_file=$2
    out_file="/tmp/${3}_spi${1}_tmp"
    resize_file $in_file $out_file $spi_no
    type=$(get_flash_first_type $spi_no)
    flashrom -p linux_spi:dev=/dev/spidev${spi_no}.0 -w $out_file -c $type
    if [ $? -ne 0 ];then
        echo "debug cmd: [flashrom -p linux_spi:dev=/dev/spidev${spi_no}.0 -w $out_file -c $type]"
        exit 1
    fi
}

erase_flash(){
    spi_no=$1
    type=$(get_flash_first_type $spi_no)
    flashrom -p linux_spi:dev=/dev/spidev${spi_no}.0 -E -c $type
    if [ $? -ne 0 ];then
        echo "debug cmd: [flashrom -p linux_spi:dev=/dev/spidev${spi_no}.0 -E -c $type]"
        exit 1
    fi
}

read_m95m02_to_file(){
    spi_no=$1
    file=$2
    eeprom_node="/sys/bus/spi/devices/spi${spi_no}.1/eeprom"
    modprobe -r at25
    modprobe at25
    dd if=${eeprom_node} of=${file} count=$m95m02_page_count bs=$m95m02_page_size
}

write_m95m02(){
    spi_no=$1
    file=$2
    out_file="/tmp/${3}_spi${1}_tmp"
    eeprom_node="/sys/bus/spi/devices/spi${spi_no}.1/eeprom"
    modprobe -r at25
    modprobe at25
    tr '\0' '\377' < /dev/zero | dd count=$m95m02_page_count bs=$m95m02_page_size of=${eeprom_node}
    resize_file $file $out_file $spi_no 'm95m02'
    dd of=${eeprom_node} if=${out_file} count=$m95m02_page_count bs=$m95m02_page_size
}

erase_m95m02(){
    spi_no=$1
    eeprom_node="/sys/bus/spi/devices/spi${spi_no}.1/eeprom"
    modprobe -r at25
    modprobe at25
    tr '\0' '\377' < /dev/zero | dd count=$m95m02_page_count bs=$m95m02_page_size of=${eeprom_node}
}

bios_flash_pre_operate() {
    curr_flash=$(boot_info.sh bios)
    if [ $curr_flash = "slave" ]; then
        wedge_power.sh off > /dev/null
    fi
}

bios_flash_post_operate() {
    curr_flash=$(boot_info.sh bios)
    if [ $curr_flash = "slave" ]; then
        wedge_power.sh on > /dev/null
    fi
}

read_spi1_dev(){
    spi_no=1
    dev=$1
    file=$2
    case ${dev} in
        ## W25Q128 , W25Q128 , W25Q257
        "BIOS" | "TH3_PCIE_FLASH" | "GB_PCIE_FLASH")
            echo "Reading flash to $file..."
            read_flash_to_file $spi_no $file
        ;;
        "SYSTEM_EE" | "BCM5389_EE")
            echo "Reading eeprom to $file..."
            # read_flash_to_file $spi_no $file
            cs=$(gpio_name2value BMC_SPI1_CS0)
            clk=$(gpio_name2value BMC_SPI1_CLK)
            miso=$(gpio_name2value BMC_SPI1_MISO)
            mosi=$(gpio_name2value BMC_SPI1_MOSI)
            at93cx6_util_py3.py --cs $cs --clk $clk --mosi $mosi --miso $miso chip read --file $file
        ;;
        "DOM_FPGA_FLASH1" | "DOM_FPGA_FLASH2")
            echo "Reading flash to $file..."
            read_flash_to_file $spi_no /tmp/fpga_fw_tmp
            in_file_sz=$(stat -c%s /tmp/fpga_fw_tmp)
            if [ -e $file ]; then
                rm $file
            fi
            pad_ff 0 $fpga_header_size $file
            dd if=/tmp/fpga_fw_tmp bs=$in_file_sz count=1 >> $file
            rm /tmp/fpga_fw_tmp
        ;;
    esac
}

write_spi1_dev(){
    spi_no=1
    dev=$1
    file=$2
    case ${dev} in
        ## W25Q128 , W25Q128 , W25Q257
        "BIOS")
            echo "Writing $file to flash..."
            bios_flash_pre_operate
            write_flash_to_file $spi_no $file $dev
            bios_flash_post_operate
        ;;
        "TH3_PCIE_FLASH" | "GB_PCIE_FLASH")
            echo "Writing $file to flash..."
            write_flash_to_file $spi_no $file $dev
        ;;
        "SYSTEM_EE" | "BCM5389_EE")
            # write_flash_to_file $spi_no $file
            echo "Writing $file to eeprom..."
            cs=$(gpio_name2value BMC_SPI1_CS0)
            clk=$(gpio_name2value BMC_SPI1_CLK)
            miso=$(gpio_name2value BMC_SPI1_MISO)
            mosi=$(gpio_name2value BMC_SPI1_MOSI)
            at93cx6_util_py3.py --cs $cs --clk $clk --mosi $mosi --miso $miso chip write --file $file
        ;;
        "DOM_FPGA_FLASH1" | "DOM_FPGA_FLASH2")
            echo "Writing $file to flash..."
            if [ -e /tmp/fpga_fw_tmp ]; then
                rm /tmp/fpga_fw_tmp
            fi
            # remove header unused of fpga bit file
            dd if=$file skip=1 bs=$fpga_header_size of=/tmp/fpga_fw_tmp
            write_flash_to_file $spi_no /tmp/fpga_fw_tmp $dev
            rm /tmp/fpga_fw_tmp
        ;;
    esac
}

erase_spi1_dev(){
    spi_no=1
    dev=$1
    file=$2
    case ${dev} in
        ## W25Q128 , W25Q128 , W25Q257
        "BIOS" | "DOM_FPGA_FLASH1" | "DOM_FPGA_FLASH2" | "TH3_PCIE_FLASH" | "GB_PCIE_FLASH")
            echo "Erasing flash..."
            erase_flash $spi_no
        ;;
        "SYSTEM_EE" | "BCM5389_EE")
            echo "Erasing eeprom..."
            # erase_flash $spi_no
            cs=$(gpio_name2value BMC_SPI1_CS0)
            clk=$(gpio_name2value BMC_SPI1_CLK)
            miso=$(gpio_name2value BMC_SPI1_MISO)
            mosi=$(gpio_name2value BMC_SPI1_MOSI)
            at93cx6_util_py3.py --cs $cs --clk $clk --mosi $mosi --miso $miso chip erase
        ;;
    esac
}

get_smb_cpld_spi_1b(){
    b0=$(cat $SMBCPLD_SYSFS_DIR/spi_1_b0)
    b1=$(cat $SMBCPLD_SYSFS_DIR/spi_1_b1)
    b2=$(cat $SMBCPLD_SYSFS_DIR/spi_1_b2)
    echo "spi_1b [ $b2 $b1 $b0 ]"
}

set_smb_cpld_spi_1b(){
    if [ $# = 3 ]; then
        echo $3 > $SMBCPLD_SYSFS_DIR/spi_1_b0
        echo $2 > $SMBCPLD_SYSFS_DIR/spi_1_b1
        echo $1 > $SMBCPLD_SYSFS_DIR/spi_1_b2
    else
        echo "Set SPI1 BIT FAILED"
    fi
}

config_spi1_pin_and_path(){
    dev=$1

    case ${dev} in
        "SYSTEM_EE")
            set_spi1_to_gpio
            echo 0 > $SMBCPLD_SYSFS_DIR/spi_1_sel
        ;;
        "BIOS")
            set_spi1_to_spi
            echo 1 > $SMBCPLD_SYSFS_DIR/spi_1_sel
        ;;
        "BCM5389_EE")
            set_spi1_to_gpio
            echo 2 > $SMBCPLD_SYSFS_DIR/spi_1_sel
        ;;
        "TH3_PCIE_FLASH" | "GB_PCIE_FLASH")
            set_spi1_to_spi
            echo 3 > $SMBCPLD_SYSFS_DIR/spi_1_sel
        ;;
        "DOM_FPGA_FLASH1")
            set_spi1_to_spi
            echo 4 > $SMBCPLD_SYSFS_DIR/spi_1_sel
        ;;
        "DOM_FPGA_FLASH2")
            set_spi1_to_spi
            echo 5 > $SMBCPLD_SYSFS_DIR/spi_1_sel
        ;;

        *)
            echo "Please enter {SYSTEM_EE, BIOS, BCM5389_EE, $SWITCH_PCIE_FLASH, DOM_FPGA_FLASH1, DOM_FPGA_FLASH2}"
            exit 1
        ;;
    esac
    echo "Config SPI1 Done."
}

operate_spi1_dev(){
    op=$1
    dev=$2
    file=$3
    ## Operate devices ##
    case ${op} in
        "read")
                read_spi1_dev $dev $file
        ;;
        "write")
                write_spi1_dev $dev $file
        ;;
        "erase")
                erase_spi1_dev $dev
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
    echo 0 > $SMBCPLD_SYSFS_DIR/spi_1_sel

    rm -rf /tmp/*_spi*_tmp
    remove_pid_file
}

ui(){
    . /usr/local/bin/openbmc-utils.sh
    op=$1
    spi=$2

    # SPI 1.0 speed
    devmem 0x1e630010 32 0x07
    # SPI 2.0 speed
    devmem 0x1e631010 32 0x407
    # SPI 2.1 speed
    devmem 0x1e631014 32 0x2007
	## Open the path to device ##
    case ${spi} in
        "spi1")
            dev=$3
            file=$4
            config_spi1_pin_and_path $dev
            operate_spi1_dev $op $dev $file
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
    echo "$program <op> spi1 <spi1 device> <file>"
    echo "  <op>          : read, write, erase"
    echo "  <spi1 device> : SYSTEM_EE, BIOS, BCM5389_EE, $SWITCH_PCIE_FLASH, DOM_FPGA_FLASH1, DOM_FPGA_FLASH2"
    echo "                  (SYSTEM_EE not support for Wedge400-C)"
    echo "Examples:"
    echo "  $program write spi1 DOM_FPGA_FLASH domfpga.bit"
    echo ""
}

check_parameter(){
    program=$(basename "$0")
    op=$1
    spi=$2
    case ${op} in
        "read" | "write" | "erase")
            ;;
        *)
            return 1
            ;;
    esac
    if [ ${spi} ] && [ ${spi} = "spi1" ]; then
        dev=$3
        file=$4

        # SYSTEM_EE disable on Wedge400-C
        if [ $dev = "SYSTEM_EE" ] && [ $MAC_TYPE = "W400C" ]; then
            return 1
        fi

        ## Check device
        case ${dev} in
            "SYSTEM_EE"|"BIOS"|"BCM5389_EE"|"TH3_PCIE_FLASH"|"GB_PCIE_FLASH"|"DOM_FPGA_FLASH1"|"DOM_FPGA_FLASH2")
                ## Check operation
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
    else
        return 1
    fi

    return 0
}

check_board_type
check_duplicate_process
check_parameter "$@"
is_par_ok=$?

if [ $is_par_ok -eq 0 ]; then
    ui "$@"
else
    usage
    exit 1
fi

