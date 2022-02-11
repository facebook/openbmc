#!/bin/bash

# YAMP's SUP MAC address is the first address of the block
# allocated from "Extended MAC Base" field in SUP's EEPROM.
# That is, SUP's MAC address is the same as "Extended MAC Base" 
# field in SUP's EEPROM
# Any error in reading SUP's EEPROM will result in empty output,
# which will be caught in the provisioning script.
weutil SUP 2>&1 | grep "MAC Base" | cut -d ' ' -f 4
