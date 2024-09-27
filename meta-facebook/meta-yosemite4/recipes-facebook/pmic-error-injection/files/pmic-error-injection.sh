#!/bin/bash

print_usage_help() {
        echo "----------------------------------------PMIC error injection script supported commands----------------------------------------"
        echo "Show error injection type: $0 --error_inj"
        echo "Error injection: $0 <slot1|slot2|slot3|slot4|slot5|slot6|slot7|slot8>"
        echo "                 --dimm <dimm_A|dimm_B|dimm_C|dimm_D|dimm_E|dimm_F|dimm_G|dimm_H|dimm_I|dimm_J|dimm_K|dimm_L>"
        echo "                 --error_inj <SWAout_OV|SWBout_OV|SWCout_OV|SWDout_OV|VinB_OV|VinM_OV|"
        echo "                 SWAout_UV|SWBout_UV|SWCout_UV|SWDout_UV|VinB_UV|Vin_switchover|high_temp|"
        echo "                 Vout_1v8_PG|high_current|current_limit|critical_temp_shutdown>"
        echo "Clear PMIC error: $0 <slot1|slot2|slot3|slot4|slot5|slot6|slot7|slot8>"
        echo "                 --dimm <dimm_A|dimm_B|dimm_C|dimm_D|dimm_E|dimm_F|dimm_G|dimm_H|dimm_I|dimm_J|dimm_K|dimm_L> --clear_error"
        echo "Example: "
        echo "         $0 slot1 --dimm dimm_A --error_inj SWBout_OV"
        echo "         $0 slot1 --dimm dimm_D --clear_error"
}

get_hex_bit_value() {
        hex_number=$1
        bit_position=$2

        declare -A hex_to_bin=(
                [0]=0000 [1]=0001 [2]=0010 [3]=0011
                [4]=0100 [5]=0101 [6]=0110 [7]=0111
                [8]=1000 [9]=1001 [a]=1010 [b]=1011
                [c]=1100 [d]=1101 [e]=1110 [f]=1111
        )

        binary=""
        for (( i=0; i<${#hex_number}; i++ )); do
                binary="${binary}${hex_to_bin[${hex_number:i:1}]}"
        done

        bit_value=${binary:$bit_position:1}
        echo "$bit_value"
}

# Main function
if [ $# -ne 1 ] && [ $# -ne 4 ] && [ $# -ne 5 ]; then
        print_usage_help
        exit 1
fi

option=$1
if [ $# -eq 1 ]; then
        if [ "$option" == "--error_inj" ]; then
                echo "Type: SWAout_OV, SWBout_OV, SWCout_OV, SWDout_OV, VinB_OV, VinM_OV,"
                echo "      SWAout_UV, SWBout_UV, SWCout_UV, SWDout_UV, VinB_UV, Vin_switchover,"
                echo "      high_temp, Vout_1v8_PG, high_current, current_limit, critical_temp_shutdown"
                exit 0
        else
                print_usage_help
                exit 1
        fi
fi

slot=$1
dimm=$3
slot_eid=0
dimm_id=0
reg=0

if [ "$slot" == "slot1" ]; then
        slot_eid=10
elif [ "$slot" == "slot2" ]; then
        slot_eid=20
elif [ "$slot" == "slot3" ]; then
        slot_eid=30
elif [ "$slot" == "slot4" ]; then
        slot_eid=40
elif [ "$slot" == "slot5" ]; then
        slot_eid=50
elif [ "$slot" == "slot6" ]; then
        slot_eid=60
elif [ "$slot" == "slot7" ]; then
        slot_eid=70
elif [ "$slot" == "slot8" ]; then
        slot_eid=80
else
        echo "Invalid slot: $slot"
        exit 1
fi

if [ "$dimm" == "dimm_A" ]; then
        dimm_id=0
elif [ "$dimm" == "dimm_B" ]; then
        dimm_id=1
elif [ "$dimm" == "dimm_C" ]; then
        dimm_id=2
elif [ "$dimm" == "dimm_D" ]; then
        dimm_id=3
elif [ "$dimm" == "dimm_E" ]; then
        dimm_id=4
elif [ "$dimm" == "dimm_F" ]; then
        dimm_id=5
elif [ "$dimm" == "dimm_G" ]; then
        dimm_id=6
elif [ "$dimm" == "dimm_H" ]; then
        dimm_id=7
elif [ "$dimm" == "dimm_I" ]; then
        dimm_id=8
elif [ "$dimm" == "dimm_J" ]; then
        dimm_id=9
elif [ "$dimm" == "dimm_K" ]; then
        dimm_id=10
elif [ "$dimm" == "dimm_L" ]; then
        dimm_id=11
else
        echo "Invalid dimm: $dimm"
        exit 1
fi

device_type=2

echo "Read write_protect status"
read_len=1
raw_cmd="pldmtool raw -m $slot_eid -d 0x80 0x3F 0x01 0x15 0xA0 0x00 0xE0 0xB1 0x15 0xA0 0x00 $dimm_id $device_type $read_len"
resp=$($raw_cmd "0x2F")

rx_data=$(echo "$resp" | grep 'Rx:' | cut -d':' -f2-)
last_byte=$(echo "$rx_data" | awk '{print $(NF)}')
bit_position=5

resp=$(get_hex_bit_value "$last_byte" "$bit_position")

if [ "$resp" -eq "1" ]; then
        echo "Write protect is enable, can't inject/clear PMIC error, last_byte: $last_byte"
        exit 1
fi

type=$4
if [ "$type" == "--clear_error" ]; then
        $raw_cmd "0x35" "0x00"
        $raw_cmd "0x39" "0x74"
        echo "Clear register R04 ~ R07 Done"
	$raw_cmd "0x14" "0x01"
        echo "Clear register R08 ~ R0B, R33 Done"
        exit 1
elif [ "$type" == "--error_inj" ]; then
	err_type=$5
        if [ "$err_type" == "SWAout_OV" ]; then
                reg="0x90"
        elif [ "$err_type" == "SWBout_OV" ]; then
                reg="0xA0"
        elif [ "$err_type" == "SWCout_OV" ]; then
                reg="0xB0"
        elif [ "$err_type" == "SWDout_OV" ]; then
                reg="0xC0"
        elif [ "$err_type" == "VinB_OV" ]; then
                reg="0xD0"
        elif [ "$err_type" == "VinM_OV" ]; then
                reg="0xE0"
        elif [ "$err_type" == "SWAout_UV" ]; then
                reg="0x98"
        elif [ "$err_type" == "SWBout_UV" ]; then
                reg="0xA8"
        elif [ "$err_type" == "SWCout_UV" ]; then
                reg="0xB8"
        elif [ "$err_type" == "SWDout_UV" ]; then
                reg="0xC8"
        elif [ "$err_type" == "VinB_UV" ]; then
                reg="0xD8"
        elif [ "$err_type" == "Vin_switchover" ]; then
                reg="0x81"
        elif [ "$err_type" == "high_temp" ]; then
                reg="0x83"
        elif [ "$err_type" == "Vout_1v8_PG" ]; then
                reg="0x84"
        elif [ "$err_type" == "high_current" ]; then
                reg="0x85"
        elif [ "$err_type" == "current_limit" ]; then
                reg="0x87"
        elif [ "$err_type" == "critical_temp_shutdown" ]; then
                reg="0x82"
        else
                echo "Invalid error type: $err_type"
                exit 1
        fi

	$raw_cmd "0x35" "$reg"
        echo "Inject PMIC error Done"
else
        echo "Invalid command type: $type"
        exit 1
fi
