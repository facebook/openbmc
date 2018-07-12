# Copyright 2015-present Facebook. All Rights Reserved.
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

GPIODIR="/sys/class/gpio"
GPIOEXPORT="$GPIODIR/export"
SHADOW_GPIO=/tmp/gpionames

gpio_dir() {
    echo "$GPIODIR/gpio$1"
}

gpio_name2value() {
    local first remaining base val
    remaining=$1
    base="${SHADOW_GPIO}/${remaining}"
    if [ -L "${base}" ]; then
        val=$(readlink -f ${base} 2>/dev/null)
        if [ -n "${val}" ]; then
            val=${val##*gpio}
            if [ -n "${val}" ]; then
                echo "$val"
                return
            fi
        fi
    fi
    val=0
    while [ -n "$remaining" ]; do
        first=${remaining:0:1}
        case "$first" in
            [[:lower:]])
                base=$(printf "%d" "'$first'")
                base=$((base - 96))
                val=$((val * 26 + base))
                ;;
            [[:upper:]])
                base=$(printf "%d" "'$first'")
                base=$((base - 64))
                val=$((val * 26 + base))
                ;;
            *)
                if [ $val -gt 0 ]; then
                    val=$((val-1))
                fi
                val=$((val * 8 + $remaining))
                break
                ;;
        esac
        remaining=${remaining:1}
    done
    echo "$val"
}

gpio_export() {
    local gpio
    gpio=$(gpio_name2value $1)
    dir=$(gpio_dir $gpio)
    if [ ! -d ${dir} ]; then
        echo $gpio > $GPIOEXPORT
    fi
    if [ $# -gt 1 ]; then
        if [ ! -d  $SHADOW_GPIO ]; then
            mkdir -p $SHADOW_GPIO
        fi
        ln -s $dir $SHADOW_GPIO/$2
    fi
}

gpio_set() {
    local gpio
    local val
    gpio=$(gpio_name2value $1)
    val=$2
    dir=$(gpio_dir $gpio)
    if [ ! -d ${dir} ]; then
        echo $gpio > $GPIOEXPORT
    fi
    if [ $val -eq 0 ]; then
        echo low > ${dir}/direction
    else
        echo high > ${dir}/direction
    fi
}

gpio_get() {
    local gpio
    local val
    local keepdirection
    gpio=$(gpio_name2value $1)
    dir=$(gpio_dir $gpio)
    keepdirection="$2"
    if [ ! -d ${dir} ]; then
        echo $gpio > $GPIOEXPORT
    fi
    if [ "$keepdirection" != "keepdirection" ]; then
        echo in > ${dir}/direction
    fi
    cat ${dir}/value
}
