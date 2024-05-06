#!/bin/bash

lockfile="/tmp/pldm-fw-upate.lock"

exec 200>"$lockfile"
flock -n 200 || { echo "PLDM firmware update is already running"; exit 1; }

SB_CPLD_ADDR="0x22"
SB_UART_MUX_SWITCH_REG="0x08"
BIC_BOOT_STRAP_SPI_VAL="0x00"
BIC_BOOT_STRAP_REG="0x0C"

SD_EEPROM_ADDR="0x54"
S3_REMAINING_WRTIE_OFFSET=("0x09" "0x00")
CPU0_REMAINING_WRTIE_OFFSET=("0x09" "0x02")
CPU1_REMAINING_WRTIE_OFFSET=("0x09" "0x04")

RAA229620_RW_TIMES=28
MP2857_TPS536C5_RW_TIMES=1000

VR_TYPE=""
VR_OFFSET=""
VR_HIGHBYTE_OFFSET=""
VR_LOWBYTE_OFFSET=""
remaining_write_times=""

show_usage() {
  echo "Usage: pldm-fw-update.sh [sd|ff|wf|sd_vr] (--rcvy <slot_id> <uart image>) (<slot_id>) <pldm image>"
  echo "       update all PLDM component  : pldm-fw-update.sh [sd|ff|wf] <pldm image>"
  echo "       update one PLDM component   : pldm-fw-update.sh [sd|ff|wf|sd_vr] <slot_id> <pldm image>"
  echo "       recovery one BIC and then update all BICs : : pldm-fw-update.sh [sd|ff|wf] --rcvy <slot_id> <uart image> <pldm image>"
  echo ""
}

check_power_on() {
	local slot=$1
	local i2c_bus=$((slot-1))
	local power_status=0

	power_status=$(gpioget "$(basename /sys/bus/i2c/devices/"$i2c_bus"-0023/*gpiochip*)" 16)
	if [ "$power_status" != "1" ]; then
		echo "Check Power : Off"
		echo "Power on before FF/WF BIC update"
		exit 255
	fi

	echo "Check Power : On"
}

recovery_bic_by_uart() {
	local slot=$1
	local cpld_uart_routing=$2
	local boot_strap_reg=$3
	local uart_image=$4
	local uart_num=$((slot < 5 ? slot - 1 :slot))
	local i2c_bus=$((slot-1))
	local ret=255

	echo "UART is ttyS$uart_num"
	echo "i2c bus is $i2c_bus"

	# Switch UART to corresponding BIC
	i2ctransfer -f -y "$i2c_bus" w2@"$SB_CPLD_ADDR" "$SB_UART_MUX_SWITCH_REG" "$cpld_uart_routing"
	ret=$?
	if [ "$ret" -ne 0 ]; then
		return $ret
	fi

	# Set BIC boot from UART
	echo "Setting BIC boot from UART"
	i2ctransfer -f -y "$i2c_bus" w2@"$SB_CPLD_ADDR" "$BIC_BOOT_STRAP_REG" "$boot_strap_reg"
	ret=$?
	if [ "$ret" -ne 0 ]; then
		return $ret
	fi

	# set UART to 115200
	/bin/stty -F "/dev/ttyS$uart_num" 115200
	ret=$?
	if [ "$ret" -ne 0 ]; then
		return $ret
	fi

	sleep 3

	echo "Doing the recovery update..."

	#echo "$uart_image"
	cat "$uart_image" > "/dev/ttyS$uart_num"

	sleep 5

	# set UART back to 57600
	/bin/stty -F "/dev/ttyS$uart_num" 57600
	ret=$?
	if [ "$ret" -ne 0 ]; then
		return $ret
	fi

	# Set BIC boot from spi
	i2ctransfer -f -y "$i2c_bus" w2@"$SB_CPLD_ADDR" "$BIC_BOOT_STRAP_REG" "$BIC_BOOT_STRAP_SPI_VAL"
	ret=$?
	if [ "$ret" -ne 0 ]; then
		return $ret
	fi

	echo "Recovery BIC is finished."
	return 0
}

wait_for_update_complete() {
	counter=0
	while true
	do
		sleep 5
		echo -ne "Waiting for updating... ${counter} sec"\\r
		progress=$(busctl get-property xyz.openbmc_project.PLDM /xyz/openbmc_project/software/"$software_id" xyz.openbmc_project.Software.ActivationProgress Progress | cut -d " " -f 2)
		if [ "${progress}" == 100 ]; then
			echo -ne \\n"Update done."\\n
			return 0
		fi
		counter=$((counter+5))
		# Over two minutes is considered timeout
		if [ "${counter}" == 300 ]; then
			echo -ne \\n"Time out. Fail"\\n
			return 255
		fi
	done
}


delete_software_id() {
	if [ "$software_id" != "" ]; then
	    echo "Delete software id. software id = $software_id"
		busctl call xyz.openbmc_project.PLDM /xyz/openbmc_project/software/"$software_id" xyz.openbmc_project.Object.Delete Delete
		ret=$?
		if [ "$ret" -ne 0 ]; then
		echo "Failed to delete software id: Exit code $ret"
		exit $ret
		fi
	fi
}

update_bic() {

	cp "$1" /tmp/images

	software_id=$(compute-software-id "$1")

	echo "software_id = $software_id"

	sleep 30

	if [ "$software_id" != "" ]; then

		if ! busctl get-property xyz.openbmc_project.PLDM /xyz/openbmc_project/software/"$software_id" xyz.openbmc_project.Software.ActivationProgress Progress >/dev/null; then
			echo "The image does not match with any devices. Please check it."
			delete_software_id
			exit 255
		fi
		sleep 10
		busctl set-property xyz.openbmc_project.PLDM /xyz/openbmc_project/software/"$software_id" xyz.openbmc_project.Software.Activation RequestedActivation s "xyz.openbmc_project.Software.Activation.RequestedActivations.Active"
		wait_for_update_complete
		return $?
	else
		echo "Fail: Miss software id."
		return 255
	fi
}

update_all_slots() {
	local is_recovery=$1
	local user_specific_slot_id=$2

	if [ "$is_recovery" == false ] && [ -z "$user_specific_slot_id" ]; then
		# 0 = true
		return 0
	else
		return 1
	fi
}

update_single_slot() {
	local slot_id=$1
	local user_specific_slot_id=$2
	local is_recovery=$3

	if [ "$is_recovery" == false ] && [ "$slot_id" == "$user_specific_slot_id" ]; then
		# 0 = true
		return 0
	else
		return 1
	fi
}

conduct_normal_update_and_updated_slot() {
	local slot_id=$1
	local user_specific_slot_id=$2
	local is_recovery=$3

	if update_single_slot "$slot_id" "$user_specific_slot_id" "$is_recovery" ; then
		# 0 = true
		return 0
	elif update_all_slots "$is_recovery" "$user_specific_slot_id"; then
		return 0
	else
		return 1
	fi
}

conduct_recovery_and_not_recovered_slot() {
	local slot_id=$1
	local user_specific_slot_id=$2
	local is_recovery=$3

	if [ "$is_recovery" == true ] && [ "$slot_id" != "$user_specific_slot_id" ]; then
		# 0 = true
		return 0
	else
		return 1
	fi
}

is_sd_bic() {
	local slot_id=$1
	local mctp_exp_id=$2

	if [[ "$slot_id" -ge 1 && "$slot_id" -le 8 && "$mctp_exp_id" == "0" ]]; then
		# 0 = true
		return 0
	else
		return 1
	fi
}


is_other_bic_updating() {
  echo "Check if other BICs are updating"

  pldm_output=$(busctl tree xyz.openbmc_project.PLDM)
  if echo "$pldm_output" | grep -qE "/xyz/openbmc_project/software/[0-9]+"; then
    echo "$pldm_output" | grep -E "/xyz/openbmc_project/software/[0-9]+"
	previous_software_id=$(busctl tree xyz.openbmc_project.PLDM |grep /xyz/openbmc_project/software/ | cut -d "/" -f 5)
    busctl get-property xyz.openbmc_project.PLDM /xyz/openbmc_project/software/"$previous_software_id" xyz.openbmc_project.Software.ActivationProgress Progress > /dev/null
	ret=$?
	if [ "$ret" -eq 0 ]; then
		echo "It can only be updated one BIC at a time. Please wait until the software update is completed."
		exit 255
	fi
  fi
}

# Function to prompt for continuation and check user input
prompt_confirmation() {
  echo "WARNING! This will automatically update all BICs."
  read -r -p "Continue? [y/N] " continue
  if [ "$continue" != "y" ]; then
    echo "Aborting on user request."
    exit 0
  fi
}

# Function to display an error message and exit the script
error_and_exit() {
  echo "Please provide $1 image"
  show_usage
  exit 255
}

get_offset_for_vr_type() {
    local vr_type=$1
    local S3_VR_ID="02"
    local CPU0_VR_ID="03"
    local CPU1_VR_ID="01"

    case "$vr_type" in
        *"$S3_VR_ID")
            echo "${S3_REMAINING_WRTIE_OFFSET[@]}"
            ;;
        *"$CPU0_VR_ID")
            echo "${CPU0_REMAINING_WRTIE_OFFSET[@]}"
            ;;
        *"$CPU1_VR_ID")
            echo "${CPU1_REMAINING_WRTIE_OFFSET[@]}"
            ;;
        *)
            echo "Failed to initialize remaining write because of unknown VR type"
            exit 255
            ;;
    esac
}

Initialize_vr_remaining_write() {
    local slot_id=$1
    local eeprom_addr=$2
    local pldm_image=$3
    local hex_dump

    hex_dump=$(hexdump -c "$pldm_image" | head -n 15 | cut -c 9- | tr -d '\n[:space:]')

    case "$hex_dump" in
        *"raa229620"*)
            remaining_write_times=$RAA229620_RW_TIMES
            ;;
        *"mp2857"*|*"tps536c5"*|*"mp2856"*)
            remaining_write_times=$MP2857_TPS536C5_RW_TIMES
            ;;
        *)
            echo "Failed to initialize remaining write because of unknown VR device"
            exit 255
            ;;
    esac

    set_vr_remaining_write_to_eeprom "$slot_id" "$eeprom_addr" "$remaining_write_times"
}

check_vr_remaining_write() {
    local i2c_bus=$(($1+15))
    local slot_id=$1
    local eeprom_addr=$2
    local pldm_image=$3
    local result

    i2ctransfer -f -y $i2c_bus w2@"$eeprom_addr" "$VR_HIGHBYTE_OFFSET" "$VR_LOWBYTE_OFFSET"

    result=$(i2ctransfer -f -y  $i2c_bus r2@"$eeprom_addr")

    IFS=' ' read -ra result_array <<< "$result"
    result="0x${result_array[0]#0x}${result_array[1]#0x}"
    remaining_write_times=$(printf "%d" "$result")

    if [ "$remaining_write_times" -eq 0 ]; then
        echo "VR device can't be updated because the remaining write times is 0"
        exit 255
    fi
}

set_vr_remaining_write_to_eeprom() {
	local i2c_bus=$(($1+15))
	local eeprom_addr=$2
	local rw_times=$3
	local hex

	hex=$(printf "%04x" "$rw_times")
	# Split the hex into two parts
	local hex1=${hex:0:2}
	local hex2=${hex:2}
	# Add the prefix "0x" to each part
	local byte1="0x$hex1"
	local byte2="0x$hex2"
	# Write the remaining write times back to EEPROM
	i2ctransfer -f -y $i2c_bus w4@"$eeprom_addr" "$VR_HIGHBYTE_OFFSET" "$VR_LOWBYTE_OFFSET" "$byte1" "$byte2"
}

# Check for minimum required arguments
[ $# -lt 2 ] && error_and_exit "PLDM"

is_rcvy=false
slot_id=
bic_name=$1
pldm_image=$2

# Determine recovery mode and check for required image files based on argument count
if [ $# -eq 5 ] && [ "$2" == "--rcvy" ]; then

  if [ "$bic_name" == "sd_vr" ]; then
	echo "VR device can't be updated in recovery mode"
	exit 255
  fi

  is_rcvy=true
  slot_id=$3
  uart_image=$4
  [ ! -f "$uart_image" ] && error_and_exit "UART"
  pldm_image=$5
elif [ $# -eq 3 ] && [[ "$2" =~ ^[1-8]+$ ]]; then
  slot_id=$2
  pldm_image=$3
  if [ "$bic_name" == "sd_vr" ]; then
		VR_TYPE=$(hexdump -n 1 -s 0x6D -e '1/1 "%02x"' "$pldm_image")
		VR_OFFSET=($(get_offset_for_vr_type "$VR_TYPE"))
		VR_HIGHBYTE_OFFSET="${VR_OFFSET[0]}"
		VR_LOWBYTE_OFFSET="${VR_OFFSET[1]}"
  fi
  pldm-package-re-wrapper bic -s "$slot_id" -f "$pldm_image"
  pldm_image="${pldm_image}_re_wrapped"
else
  prompt_confirmation
fi

[ ! -f "$pldm_image" ] && error_and_exit "PLDM"

# Execute recovery operations if in recovery mode, based on the value of bic_name
if [ "$is_rcvy" == true ]; then

  is_other_bic_updating

  # Workaround: Avoid pldm daemon blocked
  systemctl stop pldmd
  echo "Start to Recovery slot $slot_id $bic_name BIC"
  case $bic_name in
    sd)
      recovery_bic_by_uart "$slot_id" "0x04" "0x01" "$uart_image"
      ret=$?
      ;;
    ff|wf)
      check_power_on "$slot_id"
      cpld_uart_routing=$( [ "$bic_name" == "ff" ] && echo "0x05" || echo "0x01" )
      boot_strap_reg=$( [ "$bic_name" == "ff" ] && echo "0x04" || echo "0x02" )
      recovery_bic_by_uart "$slot_id" "$cpld_uart_routing" "$boot_strap_reg" "$uart_image"
      ret=$?
      pldmtool raw -m "${slot_id}0" -d 0x80 0x02 0x39 0x1 0x1 0x1 0x1 0x1
      ;;
  esac

  if [ "$ret" -ne 0 ]; then
	echo "Failed to Recovery BIC: Exit code $ret"
	exit "$ret"
  fi

  sleep 3
  echo "Restart MCTP/PLDM daemon"
  systemctl restart mctpd
  systemctl start pldmd
fi

# TODO: WF/FF need to implement the remaining write times check
if [ "$bic_name" == "sd_vr" ]; then

	if [ "$slot_id" == "" ]; then
		echo "Failed to update SD VR because <slot_id> is empty"
		exit 255
	fi

	check_vr_remaining_write "$slot_id" "$SD_EEPROM_ADDR" "$pldm_image"
	echo "remaining write before updating: $remaining_write_times"
fi

# Wating for mctp and pldm to restart
sleep 10

if ! systemctl is-active --quiet pldmd; then
    echo "STOP. PLDM service is not running. Please check pldmd status."
    exit 255
fi

if [ "$is_rcvy" == true ]; then
	pldm-package-re-wrapper bic -s "$slot_id" -f "$pldm_image"
	pldm_image="${pldm_image}_re_wrapped"
fi

busctl tree xyz.openbmc_project.MCTP
echo "Start to Update BIC"

is_other_bic_updating

update_bic "$pldm_image"
ret=$?
if [ "$ret" -ne 0 ]; then
	echo "Failed to Update BIC: Exit code $ret"
	delete_software_id
	exit $ret
fi

sleep 3

if [ "$is_rcvy" == true ]; then
	echo "slot$slot_id: Do 12V cycle"
	busctl set-property xyz.openbmc_project.State.Chassis"$slot_id" /xyz/openbmc_project/state/chassis"$slot_id" xyz.openbmc_project.State.Chassis RequestedPowerTransition s "xyz.openbmc_project.State.Chassis.Transition.PowerCycle"
	sleep 8
fi


if [ "$bic_name" == "wf" ] || [ "$bic_name" == "ff" ]; then
	# Workaround: Restart all Sentinel dome BIC for i3c hub re-init
	mapfile -t eid_arr < <(busctl tree xyz.openbmc_project.MCTP | grep /xyz/openbmc_project/mctp/1/ | cut -d "/" -f 6)
	for EID in "${eid_arr[@]}"
	do
		mctp_slot_id=$((EID/10));
		mctp_exp_id=$((EID%10));

		if is_sd_bic "$mctp_slot_id" "$mctp_exp_id"; then
			if conduct_recovery_and_not_recovered_slot "$mctp_slot_id" "$slot_id" "$is_rcvy" ||
			conduct_normal_update_and_updated_slot "$mctp_slot_id" "$slot_id" "$is_rcvy"; then
				echo "Slot $mctp_slot_id: Restart BIC"
				pldmtool raw -m "$mctp_slot_id"0 -d 0x80 0x02 0x39 0x1 0x1 0x1 0x1 0x1
			fi
		fi
	done

	sleep 3
fi

if [ "$bic_name" == "sd_vr" ]; then
	i2c_bus=$(($slot_id+15))
	i2ctransfer -f -y $i2c_bus w2@"$SD_EEPROM_ADDR" "$VR_HIGHBYTE_OFFSET" "$VR_LOWBYTE_OFFSET"
	result=$(i2ctransfer -f -y  $i2c_bus r2@"$SD_EEPROM_ADDR")
	if [ "$result" == "0xff 0xff" ]; then
		Initialize_vr_remaining_write "$slot_id" "$SD_EEPROM_ADDR" "$pldm_image"
	fi
	remaining_write_times=$((remaining_write_times-1))
	set_vr_remaining_write_to_eeprom "$slot_id" "$SD_EEPROM_ADDR" "$remaining_write_times"
	echo "remaining write after updating: $remaining_write_times"
fi

delete_software_id

echo "Done"

#unlock
flock -u 200
