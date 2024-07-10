#!/bin/bash

UARTS=("ttyUSB1" "ttyUSB2" "ttyUSB3")
BAUDS=(19200 115200)
PARITIES=("ODD" "EVEN" "NONE")

probe_dev_uart_baud_parity() {
	dev=$1
	uart=$2
	baud=$3
	parity=$4
	
  echo -ne "\rFound $dev at $uart with baud=$baud parity=$parity"
	if modbus-util --tty "$uart" -x 7 --baudrate "$baud" --parity "$parity" "${dev}0300000001" > /dev/null 2> /dev/null; then
    echo " -> FOUND"
	fi
}

probe_dev_uart_baud() {
	dev=$1
	uart=$2
	baud=$3
	for parity in "${PARITIES[@]}"; do
		probe_dev_uart_baud_parity "$dev" "$uart" "$baud" "$parity"
	done
}

probe_dev_uart() {
	dev=$1
	uart=$2
	for baud in "${BAUDS[@]}"; do
		probe_dev_uart_baud "$dev" "$uart" "$baud"
	done
}

probe_dev() {
	for uart in "${UARTS[@]}"; do
		uart_path="/dev/${uart}"
		if [ ! -e "$uart_path" ]; then
			echo "$uart_path does not exist"
			exit 1
		fi
		probe_dev_uart "$1" "$uart_path"
	done
}

# sv stop rackmond
i=0
while [ $i -lt 256 ]; do
	dev=$(printf "%02x" $i)
	probe_dev "$dev"
	i=$((i+1))
done
echo -ne "\r-----------------------------------------------------------------"
echo ""
