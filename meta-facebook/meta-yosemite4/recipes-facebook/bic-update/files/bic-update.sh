#!/bin/bash

SB_CPLD_ADDR="0x22"
SB_UART_MUX_SWITCH_REG="0x08"
BIC_BOOT_STRAP_SPI_VAL="0x00"
BIC_BOOT_STRAP_REG="0x0C"

show_usage() {
  echo "Usage: bic-updater.sh [sd|ff|wf] (--rcvy <slot_id> <uart image>) <pldm image>"
  echo ""
}

check_power_on() {
	local slot=$1
	local i2c_bus=$((slot-1))
	local power_status=0

	power_status=$(gpioget "$(basename /sys/bus/i2c/devices/"$i2c_bus"-0023/*gpiochip*)" 16)
	if [ "$power_status" != "1" ]; then
		echo "Checki Power : Off"
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

	echo "UART is ttyS$uart_num"
	echo "i2c bus is $i2c_bus"

	# Switch UART to corresponding BIC
	i2ctransfer -f -y "$i2c_bus" w2@"$SB_CPLD_ADDR" "$SB_UART_MUX_SWITCH_REG" "$cpld_uart_routing"

	# Set BIC boot from UART
	echo "Setting BIC boot from UART"
	i2ctransfer -f -y "$i2c_bus" w2@"$SB_CPLD_ADDR" "$BIC_BOOT_STRAP_REG" "$boot_strap_reg"

	# set UART to 115200
	/bin/stty -F "/dev/ttyS$uart_num" 115200
	sleep 3

	echo "Doing the recovery update..."

	#echo "$uart_image"
	cat "$uart_image" > "/dev/ttyS$uart_num"

	sleep 5

	# set UART back to 57600
	/bin/stty -F "/dev/ttyS$uart_num" 57600

	# Set BIC boot from spi
	i2ctransfer -f -y "$i2c_bus" w2@"$SB_CPLD_ADDR" "$BIC_BOOT_STRAP_REG" "$BIC_BOOT_STRAP_SPI_VAL"

	echo "Recovery BIC is finished."
}

update_bic() {

	cp "$1" /tmp/images

	sleep 1

	software_id=$(busctl tree xyz.openbmc_project.PLDM |grep /xyz/openbmc_project/software/ | cut -d "/" -f 5)
	echo "software_id = $software_id"

	sleep 1

	if [ "$software_id" != "" ]; then
		busctl set-property xyz.openbmc_project.PLDM /xyz/openbmc_project/software/"$software_id" xyz.openbmc_project.Software.Activation RequestedActivation s "xyz.openbmc_project.Software.Activation.RequestedActivations.Active"
	else
		echo "Fail: Miss software id."
	fi

	echo "Waiting for updating..."
	sleep 20
}

# Function to prompt for continuation and check user input
prompt_confirmation() {
  echo "WARNING! This tool will automatically update all BICs."
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

# Check for minimum required arguments
[ $# -lt 2 ] && error_and_exit "PLDM"

is_rcvy=false
slot_id=
bic_name=$1
pldm_image=$2

# Determine recovery mode and check for required image files based on argument count
if [ $# -eq 5 ] && [ "$2" == "--rcvy" ]; then
  is_rcvy=true
  slot_id=$3
  uart_image=$4
  [ ! -f "$uart_image" ] && error_and_exit "UART"
  pldm_image=$5
fi

[ ! -f "$pldm_image" ] && error_and_exit "PLDM"

# Comfirmation with user
prompt_confirmation

# Execute recovery operations if in recovery mode, based on the value of bic_name
if [ "$is_rcvy" == true ]; then
  # Workaround: Avoid pldm daemon blocked
  systemctl stop pldmd
  echo "Start to Recovery slot $slot_id $bic_name BIC"
  case $bic_name in
    sd)
      recovery_bic_by_uart "$slot_id" "0x04" "0x01" "$uart_image"
      ;;
    ff|wf)
      check_power_on "$slot_id"
      cpld_uart_routing=$( [ "$bic_name" == "ff" ] && echo "0x05" || echo "0x01" )
      boot_strap_reg=$( [ "$bic_name" == "ff" ] && echo "0x04" || echo "0x02" )
      recovery_bic_by_uart "$slot_id" "$cpld_uart_routing" "$boot_strap_reg" "$uart_image"
      pldmtool raw -m "${slot_id}0" -d 0x80 0x02 0x39 0x1 0x1 0x1 0x1 0x1
      ;;
  esac

  sleep 3
  echo "Restart MCTP/PLDM daemon"
  systemctl restart mctpd
  systemctl start pldmd
fi

busctl tree xyz.openbmc_project.MCTP
echo "Start to Update BIC"

update_bic "$pldm_image"

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
		if [ "$mctp_slot_id" != "$slot_id" ] && [ "$mctp_slot_id" != "0" ] && [ "$mctp_exp_id" == "0" ]; then
			echo "Slot $mctp_slot_id: Restart BIC"
			pldmtool raw -m "$mctp_slot_id"0 -d 0x80 0x02 0x39 0x1 0x1 0x1 0x1 0x1
		fi
	done

	sleep 3
fi

systemctl restart pldmd

echo "Done"
