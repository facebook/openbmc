#!/bin/sh
#
# Copyright 2020-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-bic-cached
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set Cachcing for Bridge IC info
### END INIT INFO

# GC20T5T7-278: a repeater was added on the BMC<->BIC I2C bus (i2c2) on the
# new BOM. Increase SDA data hold time (I2CD04 tHDDAT, bit[11:10]) to 2 to
# give the repeater enough margin to meet the I2C hold-time spec.
i2c2d04_val=$(devmem 0x1e78a184 2>/dev/null)
devmem 0x1e78a184 32 $(( (i2c2d04_val & ~0xC00) | 0x800 )) 2>/dev/null

echo "Setup Caching for Bridge IC info.."
/usr/bin/bic-cached > /dev/null 2>&1 &

echo "done."
