#!/bin/sh
 . /usr/local/bin/openbmc-utils.sh
 . /usr/local/fbpackages/utils/ast-functions

KVSET_CMD=/usr/bin/kv

# check the VR IC
Result=$(i2cget -f -y 17 0x45 0x1b 2>&1 1>/dev/null)

if [ -n "$Result" ]; then
    $KVSET_CMD set "VR_IC" "VI"
else
    $KVSET_CMD set "VR_IC" "IF"
fi