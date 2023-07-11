#!/bin/sh

#shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

BIOS_COMPLT="${SCMCPLD_SYSFS_DIR}/bios_post_cmplt_l"
PECI_DRIVER="/sys/bus/peci/drivers/peci-cpu"
DEV="0-30"

retry=30
while [ $retry -gt 0 ]; do
    ret=0
    bios_complt=$(head -n 1 < "$BIOS_COMPLT" 2> /dev/null)
    echo "$bios_complt"
    if [ $((bios_complt)) -ne 0 ]; then
        /usr/bin/logger -s "rebind-peci: bios not complete (remain retry $retry)"
        sleep 5;
        retry=$((retry-1))
        continue
    fi
    if [ ! -d  "$PECI_DRIVER/$DEV/peci_cpu.dimmtemp.icxd.48/hwmon/" ]; then
        ret=1
    fi
    if  [ ! -d  "$PECI_DRIVER/$DEV/peci_cpu.cputemp.icxd.48/hwmon/" ]; then
        ret=1
    fi

    if [ $((ret)) -eq 0 ]; then
        break;
    fi

    /usr/bin/logger -s "rebind-peci retry binding peci driver (remain retry $retry)"
    echo $DEV > $PECI_DRIVER/unbind
    echo $DEV > $PECI_DRIVER/bind
    sleep 5;
    retry=$((retry-1))
done; 

if [ $((ret)) -eq 0 ]; then
    /usr/bin/logger -s "rebind-peci : PECI hwmon validate Pass."
    exit 0;
else
    /usr/bin/logger -s "rebind-peci : PECI hwmon validate and rebinding failed."
    exit 1;
fi