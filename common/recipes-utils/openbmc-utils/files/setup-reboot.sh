#!/bin/sh

# flag for healthd logging reboot cause
val=$(devmem 0x1e721204 32 2>/dev/null)
devmem 0x1e721204 32 $((val | 0x101)) 2>/dev/null
