# Copyright 2015-present Facebook. All Rights Reserved.

SYSCPLD_SYSFS_DIR="/sys/class/i2c-adapter/i2c-12/12-0031"
#SCMCPLD_SYSFS_DIR="/sys/class/i2c-adapter/i2c-0/0-003e"
PWR_MAIN_SYSFS="${SYSCPLD_SYSFS_DIR}/pwr_main_n"
SCM_PRESENT_SYSFS="${SYSCPLD_SYSFS_DIR}/micro_srv_present"
PWR_BOARD_VER="${SYSCPLD_SYSFS_DIR}/board_ver"
SLOTID_SYSFS="${SYSCPLD_SYSFS_DIR}/slotid"
PWR_USRV_SYSFS="${SCMCPLD_SYSFS_DIR}/pwr_come_en"
repower=0

wedge_iso_buf_enable() {
    # TODO, no isolation buffer
    return 0
}

wedge_iso_buf_disable() {
    # TODO, no isolation buffer
    return 0
}

wedge_is_us_on() {
    local val n retries prog
    if [ $# -gt 0 ]; then
        retries="$1"
    else
        retries=1
    fi
    if [ $# -gt 1 ]; then
        prog="$2"
    else
        prog=""
    fi
    if [ $# -gt 2 ]; then
        default=$3              # value 0 means defaul is 'ON'
    else
        default=1
    fi
    #val=$(cat $PWR_USRV_SYSFS 2> /dev/null | head -n 1)
	((val=$(i2cget -f -y 0 0x3e 0x10 2> /dev/null | head -n 1)))
    if [ -z "$val" ]; then
        return $default
    elif [ `expr $val % 2` == "1" ]; then
        return 0            # powered on
    else
        return 1
    fi
}

wedge_board_type() {
    echo 'Galaxy100'
}

wedge_slot_id() {
    printf "%d\n" $(cat $SLOTID_SYSFS)
}

wedge_board_rev() {
    local val
    val=$(cat $PWR_BOARD_VER 2> /dev/null | head -n 1)
    echo $val
}

# Should we enable OOB interface or not
wedge_should_enable_oob() {
    # wedge100 uses BMC MAC since beginning
    return -1
}

galaxy100_scm_is_present() {
	#check scm if is present
	present=$(cat $SCM_PRESENT_SYSFS | head -n 1)
	if [ "$present" != "0x0" ]; then
		return 1
	fi
	return 0
}

wedge_power_on_board() {
    local val present
    # power on main power, uServer power
    val=$(cat $PWR_MAIN_SYSFS | head -n 1)
    if [ "$val" != "0x1" ]; then
        echo 1 > $PWR_MAIN_SYSFS
        sleep 2
    fi
}

wedge_power_off_board() {
    # power off main power, uServer power
    echo 0 > $PWR_MAIN_SYSFS
    sleep 2
}

come_recovery() {
	#repeater config
	repeater_config
	KR10G_repeater_config
	wedge_power_on_board
	#disable the I2C buffer to EC first
	i2cset -f -y 0 0x3e 0x18 0x07 2> /dev/null
	sleep 1
	i2cset -f -y 0 0x3e 0x10 0xfd 2> /dev/null
	usleep 11000
	i2cset -f -y 0 0x3e 0x0b 0xff 2> /dev/null
	sleep 15
	i2cset -f -y 0 0x3e 0x0b 0xfe 2> /dev/null
	usleep 11000
	i2cset -f -y 0 0x3e 0x10 0xfe 2> /dev/null
	sleep 10
	i2cset -f -y 0 0x3e 0x10 0xff 2> /dev/null
	usleep 11000
	#enable I2c buffer to EC
	i2cset -f -y 0 0x3e 0x18 0x01 2> /dev/null

	return 0
}

restore_us_com() {
	repeater_config
	KR10G_repeater_config
	wedge_power_on_board
	#reset LPC bridge
	i2cset -f -y 0 0x3e 0x31 0x0 2> /dev/null
	usleep 10000
	i2cset -f -y 0 0x3e 0x31 0x1 2> /dev/null

	i2cset -f -y 0 0x3e 0x10 0xff 2> /dev/null
	#enable I2c buffer to EC
	i2cset -f -y 0 0x3e 0x18 0x01 2> /dev/null
}

get_power_flag() {
	return $repower
}
set_power_flag() {
	repower=$1
}

i2c_write() {
	local count=0
	while [ $count -lt 10 ]; do
		i2cset -f -y $1 $2 $3 $4
		if [ $? -eq 0 ]; then
			return 0
		fi
		count=$(($count+1))
		usleep 11000
	done
	return -1
}

i2c_read() {
	local count=0
	while [ $count -lt 10 ]; do
		temp=`i2cget -f -y $1 $2 $3`
		if [ $? -eq 0 ]; then
			return $(($temp))
		fi
		count=$(($count+1))
		usleep 11000
	done
	return -1
}
#$1: bus
#$2: addr
#$3: reg
#$4: value
#$5: mask
i2c_write_verify() {
	count=0
	while [ $count -lt 10 ]; do
		#echo "i2c write $1 $2 $3 $4"
		i2c_write $1 $2 $3 $4 2> /dev/null
		if [ $? -eq 255 ]; then
			count=$(($count+1))
			continue
		fi
		((val=$4))
		((mask=$5))
		#echo -n "i2c read $1 $2 $3  "
		i2c_read $1 $2 $3 2> /dev/null
		temp=$?
		#echo -n "=$temp val=$val    "
		if [ $temp -eq 255 ]; then
			count=$(($count+1))
			continue
		fi
		((ret=temp))
		#echo -n "ret=$ret  mask=$mask  "
		if (((($ret & $mask)) == (($val)))); then
			#echo "equal"
			return 0
		fi
		count=$(($count+1))
	done
	#echo "Write bus $1 addr $2 reg $3 value $4 error!"
	return 1
}
repeater_config() {
	((val=$(i2cget -f -y 12 0x31 0x3 2> /dev/null | head -n 1)))
	((left=$(i2cget -f -y 12 0x31 0x5 2> /dev/null | head -n 1)))
	if [ $val -lt 8  -a $left -eq 0 ] || [ $val -ge 8 ]; then
		i2c_write_verify 0 0x58 0x01 0x0 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x0a 0x0 0x0
		usleep 11000
		i2c_write_verify 0 0x58 0x06 0x18 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x2c 0x00 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x33 0x00 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x3a 0x00 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x41 0x00 0xff
		usleep 11000

		i2c_write_verify 0 0x58 0x2d 0xa8 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x34 0xa8 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x3b 0xa9 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x42 0xa9 0xff
		usleep 11000

		i2c_write_verify 0 0x58 0x2e 0x0 0x1f
		usleep 11000
		i2c_write_verify 0 0x58 0x35 0x01 0x1f
		usleep 11000
		i2c_write_verify 0 0x58 0x3c 0x02 0x1f
		usleep 11000
		i2c_write_verify 0 0x58 0x43 0x02 0x1f
		usleep 11000

		i2c_write_verify 0 0x58 0x0f 0x0 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x16 0x0 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x1d 0x0 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x24 0x0 0xff
		usleep 11000

		i2c_write_verify 0 0x58 0x10 0xaf 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x17 0xaf 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x1e 0xaf 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x25 0xaf 0xff
		usleep 11000

		i2c_write_verify 0 0x58 0x11 0x0 0x1f
		usleep 11000
		i2c_write_verify 0 0x58 0x18 0x0 0x1f
		usleep 11000
		i2c_write_verify 0 0x58 0x1f 0x0 0x1f
		usleep 11000
		i2c_write_verify 0 0x58 0x26 0x0 0x1f
		usleep 11000
	else

		i2c_write_verify 0 0x58 0x01 0x0 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x0a 0x0 0x0
		usleep 11000
		i2c_write_verify 0 0x58 0x06 0x18 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x2c 0x04 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x33 0x04 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x3a 0x00 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x41 0x00 0xff
		usleep 11000

		i2c_write_verify 0 0x58 0x2d 0xab 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x34 0xab 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x3b 0xab 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x42 0xab 0xff
		usleep 11000

		i2c_write_verify 0 0x58 0x2e 0x01 0x1f
		usleep 11000
		i2c_write_verify 0 0x58 0x35 0x01 0x1f
		usleep 11000
		i2c_write_verify 0 0x58 0x3c 0x02 0x1f
		usleep 11000
		i2c_write_verify 0 0x58 0x43 0x02 0x1f
		usleep 11000

		i2c_write_verify 0 0x58 0x0f 0x01 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x16 0x01 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x1d 0x01 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x24 0x01 0xff
		usleep 11000

		i2c_write_verify 0 0x58 0x10 0xaf 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x17 0xaf 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x1e 0xaf 0xff
		usleep 11000
		i2c_write_verify 0 0x58 0x25 0xaf 0xff
		usleep 11000

		i2c_write_verify 0 0x58 0x11 0x0 0x1f
		usleep 11000
		i2c_write_verify 0 0x58 0x18 0x0 0x1f
		usleep 11000
		i2c_write_verify 0 0x58 0x1f 0x0 0x1f
		usleep 11000
		i2c_write_verify 0 0x58 0x26 0x0 0x1f
		usleep 11000
	fi
}

KR10G_repeater_config() {
	((val=$(i2cget -f -y 12 0x31 0x3 2> /dev/null | head -n 1)))
	((left=$(i2cget -f -y 12 0x31 0x5 2> /dev/null | head -n 1)))
	if [ $val -lt 8  -a $left -eq 0 ] || [ $val -ge 8 ]; then
		i2c_write_verify 0 0x59 0x06 0x18 0xff
		usleep 11000
		i2c_write_verify 0 0x59 0x0f 0x01 0xff
		usleep 11000
		i2c_write_verify 0 0x59 0x11 0x02 0x1f
		usleep 11000
		i2c_write_verify 0 0x59 0x23 0x04 0xff
		usleep 11000

		i2c_write_verify 0 0x59 0x06 0x18 0xff
		usleep 11000
		i2c_write_verify 0 0x59 0x16 0x0 0xff
		usleep 11000
		i2c_write_verify 0 0x59 0x18 0x02 0x1f
		usleep 11000
		i2c_write_verify 0 0x59 0x2d 0x04 0xff
		usleep 11000

		i2c_write_verify 1 0x59 0x06 0x18 0xff
		usleep 11000
		i2c_write_verify 1 0x59 0x0f 0x02 0xff
		usleep 11000
		i2c_write_verify 1 0x59 0x11 0x02 0x1f
		usleep 11000
		i2c_write_verify 1 0x59 0x23 0x04 0xff
		usleep 11000

		i2c_write_verify 1 0x59 0x06 0x18 0xff
		usleep 11000
		i2c_write_verify 1 0x59 0x16 0x04 0xff
		usleep 11000
		i2c_write_verify 1 0x59 0x18 0x02 0x1f
		usleep 11000
		i2c_write_verify 1 0x59 0x2d 0x04 0xff
		usleep 11000
	else
		i2c_write_verify 0 0x59 0x06 0x18 0xff
		usleep 11000
		i2c_write_verify 0 0x59 0x0f 0x01 0xff
		usleep 11000
		i2c_write_verify 0 0x59 0x11 0x04 0x1f
		usleep 11000
		i2c_write_verify 0 0x59 0x23 0x04 0xff
		usleep 11000

		i2c_write_verify 0 0x59 0x06 0x18 0xff
		usleep 11000
		i2c_write_verify 0 0x59 0x16 0x0 0xff
		usleep 11000
		i2c_write_verify 0 0x59 0x18 0x04 0x1f
		usleep 11000
		i2c_write_verify 0 0x59 0x2d 0x04 0xff
		usleep 11000

		i2c_write_verify 1 0x59 0x06 0x18 0xff
		usleep 11000
		i2c_write_verify 1 0x59 0x0f 0x02 0xff
		usleep 11000
		i2c_write_verify 1 0x59 0x11 0x02 0x1f
		usleep 11000
		i2c_write_verify 1 0x59 0x23 0x04 0xff
		usleep 11000

		i2c_write_verify 1 0x59 0x06 0x18 0xff
		usleep 11000
		i2c_write_verify 1 0x59 0x16 0x0 0xff
		usleep 11000
		i2c_write_verify 1 0x59 0x18 0x02 0x1f
		usleep 11000
		i2c_write_verify 1 0x59 0x2d 0x04 0xff
		usleep 11000
	fi
}
