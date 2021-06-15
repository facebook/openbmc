#!/bin/sh

# Setting for reboot and clear GPIO
# SCU60
val=$(devmem 0x1e6e2060 2>/dev/null)
devmem 0x1e6e2060 32 $((val | 0x1000000))
# SCU70
val=$(devmem 0x1e6e2070 2>/dev/null)
devmem 0x1e6e2070 32 $((val | 0x200))
# WDT1C
al=$(devmem 0x1e78501C 2>/dev/null)
devmem 0x1e78501C 32 $((val | 0x1000000))
# WDT20
val=$(devmem 0x1e785020 2>/dev/null)
devmem 0x1e785020 32 $((val | 0x200))

exec /usr/local/bin/setup-gpio
