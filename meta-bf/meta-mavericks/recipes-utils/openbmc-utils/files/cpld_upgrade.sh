#!/bin/sh
#
# Copyright 2014-present Facebook. All Rights Reserved.
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

upgrade_syscpld() {
  echo "Started SYSCPLD upgrade .."
  echo out > /tmp/gpionames/CPLD_JTAG_SEL/direction
  echo 1 > /tmp/gpionames/CPLD_JTAG_SEL/value
  sys_usercode=$(jbi -r -aREAD_USERCODE -gc102 -gi101 -go103 -gs100 $1 | grep -i "USERCODE code is" | awk '{ print $6 }')
  new_usercode=$(jbi -i $1 | grep -i "\"USERCODE"\" | awk -F '"' '{ print $4 }')
  if [ "${sys_usercode}" != "${new_usercode}" ];
  then
      #disable heartbeat
      i2cset -f -y 12 0x31 0x2e 0x18
      #program syscpld
      rc=$(jbi -r -aPROGRAM -gc102 -gi101 -go103 -gs100 $1 | grep -i "Success")
      if [[ $rc == *"Success"* ]];
      then
          echo "Finished SYSCPLD upgrade: Pass"
      else
          echo "Finished SYSCPLD upgrade: Fail (Program failed)"
      fi
  else
    echo "Finished SYSCPLD upgrade: No Action (Same usercode)"
  fi
}

upgrade_fancpld() {
  echo "Started FANCPLD upgrade .."
  echo out > /tmp/gpionames/CPLD_UPD_EN/direction
  echo 0 > /tmp/gpionames/CPLD_UPD_EN/value
  #get usercode from system
  sys_usercode=$(jbi -r -aREAD_USERCODE -gc77 -gi78 -go79 -gs76 $1 | grep -i "USERCODE code is" | awk '{ print $6 }')
  #get usercode from file
  new_usercode=$(jbi -i $1 | grep -i "\"USERCODE"\" | awk -F '"' '{ print $4 }')
  if [ "${sys_usercode}" != "${new_usercode}" ];
  then
      #program fancpld
      rc=$(jbi -aPROGRAM -gc77 -gi78 -go79 -gs76 $1 | grep -i "Success")
      if [[ $rc == *"Success"* ]];
      then
          echo "Finished FANCPLD upgrade: Pass"
      else
          echo "Finished FANCPLD upgrade: Fail (Program failed)"
      fi
  else
    echo "Finished FANCPLD upgrade: No Action (Same usercode)"
  fi
}

# Check the number of arguments provided, should get atleast 1 for file path
if [ $# -lt 1 ]; then
  echo "Provide FAN or SYSCPLD path"
  exit -1
fi

# Check the file path provided is a valid one.
jbcfile="$1"
if [ ! -f $jbcfile ]; then
    echo "$jbcfile does not exist"
    exit -1
fi

# Check the file path extension is .jbc
filename="$(basename $jbcfile)"
if [[ ${filename: -4} != ".jbc" ]]; then
    echo "Must pass in a .jbc file"
    exit -1
fi

# Check the file name and upgrade accordingly
if [[ $filename == *"FAN"* ]];
then
    upgrade_fancpld $jbcfile
else if [[ $filename == *"SYSCPLD"* ]];
then
    upgrade_syscpld $jbcfile
else
  echo "Provide FAN or SYSCPLD path"
  exit -1
fi
fi
