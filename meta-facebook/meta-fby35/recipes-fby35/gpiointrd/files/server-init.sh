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

### BEGIN INIT INFO
# Provides:          server-init
# Required-Start:
# Required-Stop:
# Default-Start:
# Default-Stop:
# Short-Description: Provides a service to reinitialize the server
#
### END INIT INFO

# shellcheck source=meta-facebook/meta-fby35/recipes-fby35/plat-utils/files/ast-functions
. /usr/local/fbpackages/utils/ast-functions

clear_all_vr_cache(){
  sb_vr_list=("c0h" "c4h" "ech" "c2h" "c6h" "c8h" "cch" "d0h" "96h" "9ch" "9eh" "8ah" "8ch" "8eh")
  rbf_vr_list=("b0h" "b4h" "c8h")
  for vr in "${sb_vr_list[@]}"; do
    $KV_CMD del "slot${slot_num}_vr_${vr}_crc"
    $KV_CMD del "slot${slot_num}_vr_${vr}_new_crc" persistent
  done
  for vr in "${rbf_vr_list[@]}"; do
    $KV_CMD del "slot${slot_num}_1ou_vr_${vr}_crc"
    $KV_CMD del "slot${slot_num}_1ou_vr_${vr}_new_crc" persistent
  done
}

# stop the service first
sv stop gpiod

slot_num=$1
PID=$$
PID_FILE="/var/run/server-init.${slot_num}.pid"
bus=$((slot_num-1))

# check if the script is running
[ -r "$PID_FILE" ] && OLDPID=$(cat "$PID_FILE") || OLDPID=''

# Set the current pid
echo "$PID" > "$PID_FILE"

# kill the previous running script if it is existed
if [ -n "$OLDPID" ] && (grep "server-init" /proc/$OLDPID/cmdline 1> /dev/null 2>&1) ; then
  echo "kill pid $OLDPID..."
  kill -s 9 "$OLDPID"
  pkill -KILL -f "bic-cached -. slot${slot_num}" >/dev/null 2>&1
  pkill -KILL -f "power-util slot${slot_num}" >/dev/null 2>&1
  pkill -KILL -f "setup-fan" >/dev/null 2>&1
fi
unset OLDPID

prot_bus=$((slot_num+3))
if [ "$(is_server_prsnt "$slot_num")" = "0" ]; then
  disable_server_12V_power "$slot_num"
  gpio_set_value "FM_BMC_SLOT${slot_num}_ISOLATED_EN_R" 0
  rm -f "/tmp/"*"fruid_slot${slot_num}"*
  rm -f "/tmp/"*"sdr_slot${slot_num}"*
  $KV_CMD del "slot${slot_num}_cpld_new_ver"
  $KV_CMD del "slot${slot_num}_is_m2_exp_prsnt"
  $KV_CMD del "slot${slot_num}_get_1ou_type"
  $KV_CMD del "fru${slot_num}_2ou_board_type"
  $KV_CMD del "fru${slot_num}_sb_type"
  $KV_CMD del "fru${slot_num}_sb_rev_id"
  $KV_CMD del "fru${slot_num}_is_prot_prsnt"
  clear_all_vr_cache
  i2c_device_delete "${prot_bus}" 0x50 > /dev/null 2>&1
  set_nic_power
else
  /usr/bin/sv start ipmbd_${bus} > /dev/null 2>&1
  /usr/local/bin/power-util "slot${slot_num}" 12V-on
  i2c_device_add "${prot_bus}" 0x50 24c32 > /dev/null 2>&1
  set_vf_gpio "$slot_num"
  /usr/local/bin/bic-cached -s "slot${slot_num}"
  /usr/local/bin/bic-cached -f "slot${slot_num}"
  $KV_CMD set "slot${slot_num}_sdr_thresh_update" 1
  /usr/bin/fw-util "slot${slot_num}" --version > /dev/null
fi

# reload fscd
/etc/init.d/setup-fan.sh reload

# start the services again
sv start gpiod
