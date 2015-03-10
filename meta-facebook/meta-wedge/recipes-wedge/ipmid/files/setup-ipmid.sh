#!/bin/sh
#
# Copyright 2014-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-ipmid
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set IPMI Message handler
### END INIT INFO

echo -n "Setup IPMI message handler... "
/usr/local/bin/ipmid
echo "done."
