#!/bin/bash

split32() {
    local upper lower
    upper=$(printf '0x%04x' $(( ($1 >> 16) & 0xFFFF )))
    lower=$(printf '0x%04x' $(( $1 & 0xFFFF )))
    echo "$upper $lower"
}

write32() {
  local tty addr reg upper lower
  tty=$1
  addr=$2
  reg=$3
  upper=$4
  lower=$5

  modbus-util --tty "$tty" write --baudrate 115200 "$addr" "$reg" "$upper" "$lower"
}

tty=${1:-"/dev/ttyRS485-1"}

hpr_psu_regmap="/usr/share/rackmon/registermap/orv3_hpr_psu.json"
hpr_bbu_regmap="/usr/share/rackmon/registermap/orv3_hpr_bbu.json"
hpr_psus=$(modbus-util --tty "$tty" discover --regmap "$hpr_psu_regmap" | jq -r '.[].devAddress')
hpr_bbus=$(modbus-util --tty "$tty" discover --regmap "$hpr_bbu_regmap" | jq -r '.[].devAddress')

shutdown_delay=20
psu_time_reg=0x5A
bbu_time_reg=0x65
psu_shutdown_reg=0x6D
bbu_shutdown_reg=0xED

curr=$(date +%s)
future=$((curr + shutdown_delay))
curr_data=($(split32 "$curr"))
future_data=($(split32 "$future"))

for psu in $hpr_psus; do
    write32 "$tty" "$psu" "$psu_time_reg" "${curr_data[0]}" "${curr_data[1]}"
    write32 "$tty" "$psu" "$psu_shutdown_reg" "${future_data[0]}" "${future_data[1]}"
done

for bbu in $hpr_bbus; do
    write32 "$tty" "$bbu" "$bbu_time_reg" "${curr_data[0]}" "${curr_data[1]}"
    write32 "$tty" "$bbu" "$bbu_shutdown_reg" "${future_data[0]}" "${future_data[1]}"
done
