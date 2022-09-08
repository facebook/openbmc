#!/bin/bash
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


# Store and Clear POR bit
store_clear_por() {
  if [ -f /tmp/ast_por ]; then
    return
  fi

  soc_model=$(cat /etc/soc_model 2>/dev/null)

  # default AST G4/G5
  system_reset_reg="0x1e6e203c"
  if [ "$soc_model" = "SOC_MODEL_ASPEED_G6" ]; then
      system_reset_reg="0x1e6e2074"
  fi

  # Read the SRST Rest Flag
  val=$(devmem "$system_reset_reg" 2>/dev/null)
  if [[ "$((val & 0x1))" == "1" ]]; then
      # Power ON Reset
      echo 1 > /tmp/ast_por
  else
      echo 0 > /tmp/ast_por
  fi

  # Clear Power On Reset bit
  devmem "$system_reset_reg" 32 0x1 2>/dev/null
}

# Check to see if BMC power-on-reset
is_bmc_por() {
  store_clear_por
  /bin/cat /tmp/ast_por
}
