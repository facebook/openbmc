#! /bin/bash
#
# Copyright 2015-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#

. /usr/local/fbpackages/utils/ast-functions

fru_check_list=("bmc" "bb" "slot1" "slot1 1U" "slot1 2U" "slot1 3U" "slot1 4U")

cnt_t8=0
cnt_bulk=0
cnt_cache=0
sku=

function get_sku_nume() {
	board_revision=$1
	local data

	case $sku in
		"2.1")
			data="Olympic 2.1 $board_revision T8"
			;;
		"Cache")
			data="Olympic 2.0 $board_revision RSC2 CacheStore"
			;;
		"2.0")
			data="Olympic 2.0 $board_revision T8"
			;;
		*)
			data="Olympic 2.0 $board_revision T8"
			;;
	esac

	echo $data
}

function get_fru_board_revision() {
	echo $6
}

function get_fru_name() {
	fru="$1 $2"
	local data

	case $fru in
		"slot1 1U")
			data="slot1_dev19"
			;;
		"slot1 2U")
			data="slot1_dev20"
			;;
		"slot1 3U")
			data="slot1_dev21"
			;;
		"slot1 4U")
			data="slot1_dev22"
			;;
		*)
			data=$fru
			;;
	esac

	echo $data
}

function modify_fru()
{
	for fru in "${fru_check_list[@]}"; do
		pn=$(/usr/local/bin/fruid-util $fru | grep 'Product Name')
		board_revision=$(get_fru_board_revision $pn)
		sku_name=$(get_sku_nume $board_revision)
		fru_name=$(get_fru_name $fru)

		if [[ "$pn" =~ "$fru_name" ]]; then
			continue
		else
			/usr/local/bin/fruid-util $fru --modify --PN "$sku_name" /tmp/fruid_"$fru_name".bin  > /dev/null 2>&1
			/usr/local/bin/fruid-util $fru --write /tmp/fruid_"$fru_name".bin  > /dev/null 2>&1
		fi
	done
}

function determine_sku()
{

	if [[ $cnt_bulk -gt $cnt_cache ]]; then
		max=$cnt_bulk
		sku="2.1"
	else
		max=$cnt_cache
		sku="Cache"
	fi

	if [[ $cnt_t8 -gt $max ]]; then
		sku="2.0"
	fi

	# Disable exp 3ou and 4ou normal power
	if [[ $sku -eq "Cache" ]]; then
		/usr/sbin/i2cset -y 4 0x0f 0x19 0x02 >/dev/null
	fi

	kv set system_sku "$sku"
}

function check_fru_product_name() {

	# Read product name
	for fru in "${fru_check_list[@]}"; do
		pn=$(/usr/local/bin/fruid-util $fru | grep 'Product Name')
		if [[ $pn =~ "POC" || $pn =~ "EVT" || $pn =~ "DVT" ]]; then
			return
		fi

		if [[ $pn =~ "2.1" ]]; then
			cnt_bulk=$(($cnt_bulk+1))
		elif [[ $pn =~ "CacheStore" ]]; then
			cnt_cache=$(($cnt_cache+1))
		else
			cnt_t8=$(($cnt_t8+1))
		fi
	done

	determine_sku
	modify_fru
}

function setup_system_sku() {
	server_type=$(get_server_type 1)
	if [[ "$server_type" -eq 4 ]]; then
		card_type_1ou=$(get_1ou_card_type slot1)
		if [[ "$card_type_1ou" -eq 8 ]]; then
			#dump fru from BIC
			check_fru_product_name
		fi
	fi
}

echo "Setup sku for fby35..."

setup_system_sku

echo "Done."
