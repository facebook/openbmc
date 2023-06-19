#!/bin/bash
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
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
# This script resets the OOB switch BCM5387, and it's designed to reload
# the new settings from OOB switch EEPROM without power cycling the whole
# chassis.
#
# WARNING:
#   1. Both uServer and OpenBMC OOB connection would be down for a few
#      seconds when the command is executed.
#   2. DO NOT run the script from ssh terminal because of #1: run the
#      command from OpenBMC console instead.
#

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

SYSCPLD_OOB_RST="${SYSCPLD_SYSFS_DIR}/oob_bcm5387_rst_n"

echo "This command disrupts uServer and OpenBMC OOB connection!"
read -r -t 8 -p "Do you want to continue? [y/n] " user_input
if [ "$user_input" != "y" ] && [ "$user_input" != "Y" ] ; then
    exit 0
fi

if [ ! -L "${SHADOW_GPIO}/SWITCH_EEPROM1_WRT" ]; then
    gpio_export_by_name "$ASPEED_GPIO" GPIOE2 SWITCH_EEPROM1_WRT
fi
gpio_set_direction SWITCH_EEPROM1_WRT "in"

# The OOB switch enters reset state after below command!!!
logger -p user.crit "Reset OOB switch BCM5387.."
echo "Reset OOB switch BCM5387.."
echo 0 > "$SYSCPLD_OOB_RST"

# Now let's bring the OOB switch out of reset after small delay.
sleep 2
echo "Bring BCM5387 out of reset.."
echo 1 > "$SYSCPLD_OOB_RST"

gpio_unexport SWITCH_EEPROM1_WRT
exit 0
