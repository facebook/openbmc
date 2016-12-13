#!/bin/sh

source /usr/local/fbpackages/utils/ast-functions

status_switch() {
	prompt=1
	while getopts "f" opts; do
		case $opts in
			f)
				prompt=0
				shift
				;;
			*)
				prompt=1
				;;
		esac
	done
	if [ "$1" = "mdio" ]; then
		gpio_export R6
		gpio_export R7
		gpio_set E2 1
	elif [ "$1" = "eeprom" ]; then
		gpio_set E2 0
	else
		echo "Mode doesn't support!"
		return 1
	fi
	if [ $prompt -eq 1 ]; then
		printf "Need to reset BCM5389(Y/n) "
		read confirm
		if [ "$confirm" = "Y" ] || [ "$confirm" = "y" ]; then
			printf "Reset......"
			i2cset -f -y 12 0x31 0x14 0xfb
			sleep 1
			i2cset -f -y 12 0x31 0x14 0xff
			printf "OK\n"
		fi
	else
		printf "Reset......"
		i2cset -f -y 12 0x31 0x14 0xfb
		sleep 1
		i2cset -f -y 12 0x31 0x14 0xff
		printf "OK\n"
	fi

	return 0
}

command="$1"
case "$command" in
	--mode)
		shift
		status_switch $@
		;;
	*)
		bcm5396_util.py $@
		;;
esac
