# Copyright 2015-present Facebook. All Rights Reserved.

SYSCPLD_SYSFS_DIR="/sys/class/i2c-dev/i2c-13/device/13-003e"
SLOTID_SYSFS="${SYSCPLD_SYSFS_DIR}/slotid"

wedge_board_type() {
    echo 'CMM'
}

wedge_slot_id() {
    val=$(head -n 1 $SLOTID_SYSFS)
    if [ "${val}" = "0x1" ]; then
        echo c1
    else
        echo c2
    fi
}
