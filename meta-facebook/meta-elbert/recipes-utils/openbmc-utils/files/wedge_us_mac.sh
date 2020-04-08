#!/bin/bash

# ELBERT's SUP MAC address is the first address of the block
# allocated from "Extended MAC Base" field in SUP's EEPROM.
weutil SUP 2>&1 | grep "MAC Base" | cut -d ' ' -f 4
