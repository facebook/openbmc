#!/bin/bash
#
# Copyright 2019-present Facebook. All Rights Reserved.
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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

. /usr/local/bin/openbmc-utils.sh

SMB_DIR=$SMBCPLD_SYSFS_DIR
SCM_CMD=$SMB_DIR'/scm_present_int_status'
FAN_CMD=$FCMCPLD_SYSFS_DIR


function usage
{
  program=`basename "$0"`
  echo "Usage:"
  echo "     $program "
  echo "     $program <TYPE>"
  echo "     <TYPE>: scm/fan/psu"
}

# Will return <key> : <val>
# eg - scm : 1
function get_presence
{
  local file=$1 key=$2
  val=$(head -n1 $file | awk '{print substr($0,0,3) }' | sed 's/.x//')
  if [ $val -eq 0 ]; then
      val=1
  else
      val=0
  fi
  echo $key ":" $val
}

# SCM
function get_scm_presence
{
  key='scm'
  get_presence $SCM_CMD $key
}

# FANs
function get_fan_presence
{
  local num=1
  while [ $num -le 4 ]; do
      key='fan'$num
      file=$FAN_CMD'/fan'$num'_present'
      get_presence $file $key
      (( num++ ))
  done
}

# Power supplies
function get_psu_presence
{
  for i in {1..2}; do
      key='psu'${i}
      file=$SMB_DIR'/psu_present_'${i}'_N_int_status'
      get_presence $file $key
  done
}

# get all info
if [[ $1 = "" ]]; then
  echo "CARDS"
  get_scm_presence

  echo "FANS"
  get_fan_presence

  echo "POWER SUPPLIES"
  get_psu_presence

else
  if [[ $1 = "scm" ]]; then
    get_scm_presence
  elif [[ $1 = "fan" ]]; then
    get_fan_presence
  elif [[ $1 = "psu" ]]; then
    get_psu_presence
  else
    usage
    exit -1
  fi
fi
