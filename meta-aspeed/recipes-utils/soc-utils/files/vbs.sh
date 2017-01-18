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

VBS="0x1E720100"

vbs_addr() {
    echo $(($VBS + $1))
}

main() {
  ROM_EXEC_ADDRESS=$(devmem $(vbs_addr 0x4) 32)
  echo "ROM executed from:      $ROM_EXEC_ADDRESS"

  UBOOT_EXEC_ADDRESS=$(devmem $(vbs_addr 0x0) 32)
  echo "U-Boot executed from:   $UBOOT_EXEC_ADDRESS"

  ROM_KEYS=$(devmem $(vbs_addr 0x8) 32)
  echo "ROM KEK certificates:   $ROM_KEYS"

  SUBORDINATE_KEYS=$(devmem $(vbs_addr 0xC) 32)
  echo "U-Boot certificates:    $SUBORDINATE_KEYS"

  # Flags
  FORCE_RECOVERY=$(devmem $(vbs_addr 0x12) 8)
  echo "Flags force_recovery:   $FORCE_RECOVERY"

  HARDWARE_ENFORCE=$(devmem $(vbs_addr 0x13) 8)
  echo "Flags hardware_enforce: $HARDWARE_ENFORCE"

  SOFTWARE_ENFORCE=$(devmem $(vbs_addr 0x14) 8)
  echo "Flags software_enforce: $SOFTWARE_ENFORCE"

  RECOVERY_BOOT=$(devmem $(vbs_addr 0x15) 8)
  echo "Flags recovery_boot:    $RECOVERY_BOOT"

  RECOVERY_RETRIES=$(devmem $(vbs_addr 0x16) 8)
  echo "Flags recovery_retired: $RECOVERY_RETRIES"

  # Errors
  ERROR_TYPE=$(devmem $(vbs_addr 0x17) 8)
  ERROR_CODE=$(devmem $(vbs_addr 0x18) 8)
  echo ""
  echo "Status type=$ERROR_TYPE code=$ERROR_CODE"
}

main
