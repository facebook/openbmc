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

check_flash_info()
{
    spi_no=$1
    flashrom -p "linux_spi:dev=/dev/spidev$spi_no.0"
}

get_flash_first_type()
{
    ori_str=$(check_flash_info "$spi_no")
    type=$(echo $ori_str | cut -d '"' -f 2)
    if [ "$type" ];then
        echo "$type"
        return 0
    else
        echo "Get flash type error: [$ori_str]"
        exit 1
    fi
}

read_spi1_dev(){
    spi_no=1
    dev=$1
    file=$2
    echo "Reading ${dev} to $file..."
    type=$(get_flash_first_type "$spi_no")
    flashrom -p linux_spi:dev=/dev/spidev1.0 -r "$file" -c "$type"
}

write_spi1_dev(){
    spi_no=1
    dev=$1
    file=$2
    echo "Writing $file to ${dev}..."
    type=$(get_flash_first_type $spi_no)

    tmpfile="$file"_ext
    flashsize=$(flashrom -p linux_spi:dev=/dev/spidev1.0 | grep -i kB | xargs echo | cut -d '(' -f 2 | cut -d ' ' -f 0)
    targetsize=$((flashsize * 1024))

    cp "$file" "$tmpfile"

    #extend the temp-file
    filesize=$(stat -c%s $tmpfile)
    add_size=$((targetsize - filesize))
    dd if=/dev/zero bs="$add_size" count=1 | tr "\000" "\377" >> "$tmpfile"
    flashrom -p linux_spi:dev=/dev/spidev1.0 -w "$tmpfile" -c "$type"
}

erase_spi1_dev(){
    spi_no=1
    type=$(get_flash_first_type $spi_no)
    flashrom -p linux_spi:dev=/dev/spidev1.0 -E -c "$type"
}

config_spi1_pin_and_path(){
    dev=$1
    case ${dev} in
        "BIOS")
            echo "[Setup] BIOS"
            echo 0x01 > "$SMBCPLD_SYSFS_DIR/spi_1_sel"
        ;;
        "GB_PCIE_FLASH")
            echo "[Setup] GB_PCIE_FLASH"
            echo 0x03 > "$SMBCPLD_SYSFS_DIR/spi_1_sel"
        ;;
        "DOM_FPGA_FLASH1")
            echo "[Setup] DOM_FPGA_FLASH1"
            echo 0x04 > "$SMBCPLD_SYSFS_DIR/spi_1_sel"
        ;;
        "DOM_FPGA_FLASH2")
            echo "[Setup] DOM_FPGA_FLASH2"
            echo 0x05 > "$SMBCPLD_SYSFS_DIR/spi_1_sel"
        ;;
        *)
            echo "Please enter {BIOS, GB_PCIE_FLASH, DOM_FPGA_FLASH1, DOM_FPGA_FLASH2}"
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
                read_spi1_dev "$dev" "$file"
        ;;
        "write")
                write_spi1_dev "$dev" "$file"
        ;;
        "erase")
                erase_spi1_dev "$dev"
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
        *)
            echo "No such SPI bus."
            return 1
        ;;
    esac
}

usage(){
    program=$(basename "$0")
    echo "Usage:"
    echo "  $program <op> <spi1> <spi device> <file>"
    echo "    <op>          : read, write, erase"
    echo "    <spi1 device> : BIOS, GB_PCIE_FLASH, DOM_FPGA_FLASH1, DOM_FPGA_FLASH2"
    echo ""
    echo "Examples:"
    echo "  $program write spi1 BIOS bios.bin"
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
                "BIOS"|"GB_PCIE_FLASH"|"DOM_FPGA_FLASH1"|"DOM_FPGA_FLASH2")
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

