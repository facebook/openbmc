#!/bin/bash

# This is a workaround for preventing BMC's EID hot join issue.
# Which would cause corresponding bus been stuck by pldmd when the device's EID is still exist but the device has lost connection.

SCRIPT_VERSION="4.0"
for arg in "$@"; do
    if [ "$arg" == "-v" ]; then
        echo "Script version: $SCRIPT_VERSION"
        exit 0
    fi
done

STATUS_SUCCESS=0
STATUS_FAILURE=1
STATUS_IS_RUNNING=2

# Check if eid-auto-discovery.sh is running
if pgrep -f eid-auto-discovery.sh | grep -v $$ > /dev/null
then
    echo "eid-auto-discovery.sh is already running."
    exit $STATUS_IS_RUNNING
fi

# Create an associative array to store the map
declare -A is_wf_bic_ready

is_wf_bic_ready["1"]=0
is_wf_bic_ready["2"]=0
is_wf_bic_ready["3"]=0
is_wf_bic_ready["4"]=0
is_wf_bic_ready["5"]=0
is_wf_bic_ready["6"]=0
is_wf_bic_ready["7"]=0
is_wf_bic_ready["8"]=0

while [ $wait_time -le 15 ]; do
    if systemctl is-active --quiet mctpd.service; then
        echo "mctpd.service is active."
        break
    fi
    sleep 1
    wait_time=$(($wait_time+1))
done


while true; do
    for i in {1..8}; do
        slot_bus=$((i - 1))
        bic_status=$(gpioget $(basename "/sys/bus/i2c/devices/${slot_bus}-0024/"*gpiochip*) 1 2>&1)

        if [[ ! $bic_status =~ ^[0-9]+$ ]]; then
            continue
        fi

        if [ "$bic_status" -eq 1 ]; then
            busctl call xyz.openbmc_project.MCTP /xyz/openbmc_project/mctp au.com.CodeConstruct.MCTP SetupEndpointByConfigPath s /xyz/openbmc_project/inventory/system/board/Yosemite_4_Sentinel_Dome_Slot_${i}/BIC 2>/dev/null
            output=$(pldmtool raw -m ${i}0 -d 0x80 0x3f 0x1 0x15 0xa0 0x0 0x18 0x52 0x9 0x42 1 0x8)
            rx_line=$(echo "$output" | grep "Rx:")
            sb_revision=$(echo "$rx_line" | awk '{print $13}')
            if [ "$sb_revision" == "00" ] || [ "$sb_revision" == "01" ]; then
                    output=$(pldmtool raw -m ${i}0 -d 0x80 0x02 0x3A 0x4C 0xFF)
            else
                    output=$(pldmtool raw -m ${i}0 -d 0x80 0x02 0x3A 0x2C 0xFF)
            fi
            rx_line=$(echo "$output" | grep "Rx:")
            wf_ready=$(echo "$rx_line" | awk '{print $13}')
	    is_wf_eid_exist=$(busctl tree xyz.openbmc_project.MCTP | grep ${i}2)
            if [ "$wf_ready" == "02" ] && [ -z "$is_wf_eid_exist" ]; then
                busctl call xyz.openbmc_project.MCTP /xyz/openbmc_project/mctp au.com.CodeConstruct.MCTP SetupEndpointByConfigPath s /xyz/openbmc_project/inventory/system/board/Yosemite_4_Wailua_Falls_Slot_${i}/BIC 2>/dev/null
                is_wf_bic_ready["${i}"]=1
            elif [ "$wf_ready" == "01" ] && [ -n "$is_wf_eid_exist" ]; then
                busctl call xyz.openbmc_project.MCTP /xyz/openbmc_project/mctp/1/${i}2 au.com.CodeConstruct.MCTP.Endpoint Remove 2>/dev/null
                is_wf_bic_ready["${i}"]=0
            fi
        elif [ "$bic_status" -eq 0 ]; then
            sleep 1
            ac_status=$(gpioget $(basename "/sys/bus/i2c/devices/28-0021/"*gpiochip*) ${slot_bus} 2>&1)
            if [ "$ac_status" -eq 1 ]; then
                busctl call xyz.openbmc_project.MCTP /xyz/openbmc_project/mctp/1/${i}0 au.com.CodeConstruct.MCTP.Endpoint Remove 2>/dev/null
                busctl call xyz.openbmc_project.MCTP /xyz/openbmc_project/mctp/1/${i}2 au.com.CodeConstruct.MCTP.Endpoint Remove 2>/dev/null
                is_wf_bic_ready["${i}"]=0
            fi
        fi
    done

    sleep 5
done
