#!/bin/sh

# Address of SRAM that is used to store "reboot by command" flag. 
SRAM_BMC_REBOOT_BY_CMD="0x10015C04"

# Set 0x101 as flag for healthd logging reboot cause.
val=$(devmem "$SRAM_BMC_REBOOT_BY_CMD" 32 2>/dev/null)
devmem "$SRAM_BMC_REBOOT_BY_CMD" 32 $((val | 0x101)) 2>/dev/null

