#!/bin/bash
#
# Copyright 2018-present Facebook. All Rights Reserved.
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

source /usr/local/bin/openbmc-utils.sh

SCM_CMD=$SMBCPLD_SYSFS_DIR'/scm_presnt_status'
PIM_CMD=$SMBCPLD_SYSFS_DIR'/*prsnt_n_status'

function usage
{
  program=`basename "$0"`
  echo "Usage:"
  echo "     $program "
  echo "     $program <TYPE>"
  echo "     <TYPE>: scm/pim/fan/psu"
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

# PIMs
function get_pim_presence
{
    local count=1
    for file in $PIM_CMD; do
        key='pim'$count
        get_presence $file $key
        (( count++ ))
    done
}

# FANs
function get_fan_presence
{
    local num=1
    while [ $num -le 4 ]; do
        count=$(((${num} * 2 ) - 1))
        key='fan'$count
        file=$TOP_FCMCPLD_SYSFS_DIR'/fantray'$num'_present'
        get_presence $file $key
        count=$((${num} * 2 ))
        key='fan'$count
        file=$BOTTOM_FCMCPLD_SYSFS_DIR'/fantray'$num'_present'
        get_presence $file $key
        (( num++ ))
        (( count++ ))
    done
}

# Power supplies
function get_psu_presence
{
    for i in {2..1}; do
        count=$((3 - ${i}))
        key='psu'$count
        file=$SMBCPLD_SYSFS_DIR'/psu_L'${i}'_present_L'
        get_presence $file $key
    done
    for i in {2..1}; do
        count=$((5 - ${i}))
        key='psu'$count
        file=$SMBCPLD_SYSFS_DIR'/psu_R'${i}'_present_L'
        get_presence $file $key
    done
}

# get all info
if [[ $1 = "" ]]; then
  echo "CARDS"
  get_scm_presence
  get_pim_presence

  echo "FANS"
  get_fan_presence

  echo "POWER SUPPLIES"
  get_psu_presence

else
  if [[ $1 = "scm" ]]; then
    get_scm_presence
  elif [[ $1 = "pim" ]]; then
    get_pim_presence
  elif [[ $1 = "fan" ]]; then
    get_fan_presence
  elif [[ $1 = "psu" ]]; then
    get_psu_presence
  else
    usage
  fi
fi
