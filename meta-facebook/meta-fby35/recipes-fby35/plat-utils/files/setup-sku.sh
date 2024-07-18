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

cnt_t8_0=0 #2.0
cnt_t8_2=0 #2.2
sku=

function determine_sku()
{

	if [[ $cnt_t8_0 -gt $cnt_t8_2 ]]; then
		sku="2.0"
	else
		sku="2.2"
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

		if [[ $pn =~ "2.2" ]]; then
			cnt_t8_2=$(($cnt_t8_2+1))
		else
			cnt_t8_0=$(($cnt_t8_0+1))
		fi
	done

	determine_sku
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
