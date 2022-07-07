#!/bin/bash
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

#shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

get_version () {
    length=$( stat -c%s "$1" )
    ((i = length - 4))
    ((major_offset_found = 0))
    while [[ $i -ne 0 ]]
    do
        var=$( dd skip=$i count=4 if="$1" bs=1 2> /dev/null )
        if [ "$var" = "fa7f" ];then
            if [ $major_offset_found -eq 0 ]; then
                ((major_offset_found = 1))
                ((major_offset = i + 4))
            else
                ((minor_offset = i + 4))
                break
            fi
        fi
        ((i = i - 1))
    done
    major=$( dd skip=$major_offset count=2 if="$1" bs=1 2> /dev/null)
    minor=$( dd skip=$minor_offset count=2 if="$1" bs=1 2> /dev/null)
    echo "version is $major.$minor"
}

#Port 0 - I210 (x86 side)
#Port 1 - 1G PHY (front panel) ethernet port
#Port 5 - BMC CPU
monitored_ports=("0" "1" "5")

start_monitoring() {
    reset_count=0
    while : ; do
        sleep 3
        if ! echo "status" > /dev/switch 2> /dev/null; then
            # A device busy error may be seen if more than
            # one instance of this script is run at the same
            # time
            continue
        fi
        ports_status=$( tr -d '\0' < /dev/switch )
        if [ "$ports_status" ]; then
            # Loop over the "ports_status" line by line, each line has
            # the status of one port. If that port status was not read
            # before (i.e first check) then it is printed out, but if
            # the status was read before then it is only printed
            # if the status has changed since the last check.
            # Sample "ports_status" output:
                  #Link status for port 0 is UP
                  #Link status for port 1 is UP
                  #Link status for port 2 is DOWN
                  #Link status for port 3 is DOWN
                  #Link status for port 4 is DOWN
                  #Link status for port 5 is UP
                  #Link status for port 6 is DOWN
            # The "all_ports_down" flag is set to true before the ports
            # status is checked, then if any of the ports is UP the flag
            # is set to false. If "all_ports_down" flag is true for 2
            # polling intervals (i.e. all ports are down for more than
            # three seconds, then the switch will be reset to recover for a
            # maximum three consecutive attempts.
            all_ports_down=1
            while read -r line
            do
                port=$( echo "$line" | cut -d ' ' -f 5)
                status=$( echo "$line" | cut -d ' ' -f 7)
                if [[ -z "$port" ]]; then
                    continue
                fi
                if [[ " ${monitored_ports[*]} " =~ $port ]]; then
                    if [ -z "${port_status[$port]}" ]; then
                        printf '%(%Y-%m-%d %H:%M:%S)T\n' -1
                        printf "port %s link status is %s\n" "$port" "$status"
                        port_status["$port"]=$status
                    else
                        if [[ "${port_status[$port]}" != "$status" ]]; then
                            printf '%(%Y-%m-%d %H:%M:%S)T\n' -1
                            printf "port %s link status changed to %s\n" "$port" "$status"
                        fi
                    fi
                    port_status["$port"]=$status
                    if [ "$status" = "UP" ]; then
                        # For testing only, to trigger a switch reset
                        if [ ! -f /tmp/link_down_test ]; then
                            all_ports_down=0
                        fi
                    fi
                fi
            done < <(printf '%s\n' "$ports_status")

            if [ "$all_ports_down" -eq 1 ]; then
                if [ -n "$reset_pending" ] && [ "$reset_pending" -eq 1 ]; then
                    printf '%(%Y-%m-%d %H:%M:%S)T\n' -1
                    printf "all ports are down, resetting the switch to recover\n"
                    # RESET SWITCH
                    oob_switch_reset
                    ((reset_count+=1))
                    # Testing shows that MDIO access fails for 3 or 4
                    # seconds after switch reset
                    sleep 6
                    reset_pending=0
                else
                    if [ "$reset_count" -eq 3 ]; then
                        printf '%(%Y-%m-%d %H:%M:%S)T\n' -1
                        printf "all ports are down, exhausted all %s recovery attempts\n" "$reset_count"
                        # May issue router power cycle at the point
                        exit 0
                    fi
                    reset_pending=1
                fi
            else
                reset_count=0
                reset_pending=0
            fi
        fi
    done
}

gb_util() {

    if [ ! -f /sys/bus/i2c/devices/8-0050/eeprom ]; then
        echo "/sys/bus/i2c/devices/8-0050/eeprom does not exist"
        return 1
    fi

    if [ "$1" = "upgrade" ];then
        if [ -f "$2" ]; then
            oob_switch_eeprom_select 
            cat < "$2" 1<> /sys/bus/i2c/devices/8-0050/eeprom
            oob_switch_eeprom_deselect 
        else 
            echo "$2 does not exist"
        fi
    fi

    if [ "$1" = "enable" ];then
        oob_switch_eeprom_select 
    fi

    if [ "$1" = "disable" ];then
        oob_switch_eeprom_deselect 
    fi

    if [ "$1" = "reset" ];then
        oob_switch_reset 
    fi

    if [ "$1" = "erase" ];then
        printf '\xff' > /tmp/erasefile
        oob_switch_eeprom_select 
        ((i = 0))
        while [[ $i -lt 512 ]]
        do
            dd seek=$i count=1 if=/tmp/erasefile of=/sys/bus/i2c/devices/8-0050/eeprom bs=1 2> /dev/null
            ((i = i +1))
        done
        oob_switch_eeprom_deselect 
        rm /tmp/erasefile
    fi

    if [ "$1" = "eeprom_version" ];then
        oob_switch_eeprom_select
        hexdump -ve '1/1 "%.2x"' /sys/bus/i2c/devices/8-0050/eeprom > /tmp/eeprom_data
        oob_switch_eeprom_deselect
        get_version /tmp/eeprom_data
        rm /tmp/eeprom_data
    fi

    if [ "$1" = "packaged_version" ];then
        hexdump -ve '1/1 "%.2x"' "$2" > /tmp/eeprom_data
        get_version /tmp/eeprom_data
        rm /tmp/eeprom_data
    fi

    if [ "$1" = "check" ];then
        i2cset -f -y 12 0x31 0x0c
        i2cset -f -y 12 0x32 0x00
        i2cset -f -y 12 0x33 0x00
        i2cset -f -y 12 0x34 0x00
        i2cset -f -y 12 0x30 0x1
        access=$( i2cget -f -y 12 0x35 )
        if [ $(( access & 0x4 )) -eq $(( 0x0 )) ]; then
            echo "access is disabled"
        else
            echo "access is enabled"
        fi
    fi

    if [ "$1" = "status" ];then
        if [ ! -c "/dev/switch" ]; then
            echo "/dev/switch does not exist, mv88e6320 kernel module needed"
            return 1
        fi
        if echo "status" > /dev/switch 2> /dev/null; then
            cat /dev/switch
        fi
    fi

    if [ "$1" = "statistics" ];then
        if [ ! -c "/dev/switch" ]; then
            echo "/dev/switch does not exist, mv88e6320 kernel module needed"
            return 1
        fi
        if echo "statistics" > /dev/switch 2> /dev/null; then
            cat /dev/switch
        fi
    fi

    if [ "$1" = "monitor" ];then
        if [ ! -c "/dev/switch" ]; then
            echo "/dev/switch does not exist, mv88e6320 kernel module needed"
            return 1
        fi
        start_monitoring
    fi
}

case "$1" in
    "--help" | "?" | "-h")
        program=$(basename "$0")
        echo "Usage: switch firmware upgrade command"
        echo "  <$program upgrade image> enable access to switch EEPROM, upgrade FW with provided image, then disable access"
        echo "  <$program enable> enable access to switch EEPROM for firmware upgrade"
        echo "  <$program disable> disable access to switch EEPROM"
        echo "  <$program reset> reset switch, can be done after FW upgrade to activate new image"
        echo "  <$program erase> erase the FW image from the switch EEPROM, switch will boot with HW configuration after next reset"
        echo "  <$program eeprom_version> get the FW version from the switch EEPROM"
        echo "  <$program packaged_version image> get the FW version from the switch FW image file" 
        echo "  <$program check> check access status to switch EEPROM"
        echo "  <$program status> get all ports status (UP or DOWN)"
        echo "  <$program statistics> get statisitcs for ports that are UP"
        echo "  <$program monitor> start monitoring and recovery. Switch will be reset if all used ports are DOWN for more than 3 seconds"
    ;;

    *)
        gb_util "$1" "$2"
    ;;
esac
