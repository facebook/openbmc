#!/bin/sh
echo  "Setup SCU "

# Unluck
devmem 0x1e6e2000 32 0x1688a8a8

# Error message when booting.
# ERROR:root:Failed to configure "GPIOM6" for "GPIO_M6": Not able to unsatisfy an AND condition
# ERROR:root:Failed to configure "GPIOM7" for "GPIO_M7": Not able to unsatisfy an AND condition
# SET SCU84[31:30]=0
devmem 0x1e6e2084 32 0x00c0f000

