#!/bin/bash

# ELBERT's SCM MAC address is the first address of the block
# allocated from "Extended MAC Base" field in SCM's EEPROM.
weutil SCM 2>&1 | grep "MAC Base" | cut -d ' ' -f 4
