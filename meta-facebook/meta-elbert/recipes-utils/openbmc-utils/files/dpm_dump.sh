#!/bin/bash

dpm_path="/sys/bus/i2c/drivers/ucd9000/3-004e"
logfile="/mnt/data1/log/dpm_log"

trap cleanup INT TERM QUIT EXIT

REBIND=false

if [ -d "$dpm_path" ]; then
    REBIND=true
    echo "# Unbinding SWITCHCARD DPM from ucd9000 driver"
    echo "3-004e" > /sys/bus/i2c/drivers/ucd9000/unbind
fi

echo "Reading DPM logs..."

log=''
log="$log{'Date':'$(date -u "+%Y-%m-%d %T")'"
log="$log,'Log':'$(i2cget -y 3 0x4e 0xea i | tail -n 1)'"

eb="$(i2cget -y 3 0x4e 0xeb w)"

len="${eb:2:2}"

for (( i = 0 ; i < "$len" ; i++ ))
do
  i2cset -y 3 0x4e 0xeb 0x"$len""$(printf '%02x' "$i")" w
  man="$(i2cget -y 3 0x4e 0xec i | tail -n 1)"
  exp="$(i2cget -y 3 0x4e 0x20 | tail -n 1)"
  log="$log,'detail_$i':{'m':'$man','e':'$exp'}"
done

log="$log}"

echo "$log" >> "$logfile"

echo "Clearing DPM logs..."
i2cset -y 3 0x4e 0xea 0 0 0 0 0 0 0 0 0 0 0 0 0 0 s

cleanup() {
    if [ "$REBIND" = true ]; then
        echo "# Re-Binding SWITCHCARD DPM to ucd9000 driver"
        echo "3-004e" > /sys/bus/i2c/drivers/ucd9000/bind
    fi
}
