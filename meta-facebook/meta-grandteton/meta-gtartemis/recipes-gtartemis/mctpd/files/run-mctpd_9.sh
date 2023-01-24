#!/bin/sh
i2cset -y 9 0x71 0 0x05 # MUX to Channel 1 (MEB BIC)
exec /usr/local/bin/mctpd smbus 9 0x10

