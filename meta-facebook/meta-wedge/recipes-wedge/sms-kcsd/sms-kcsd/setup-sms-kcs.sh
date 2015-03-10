#!/bin/sh
#
# Copyright 2014-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-sms-kcs
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set SMS KCS handler
### END INIT INFO

echo -n "Setup SMS KCS message handler... "
/usr/local/bin/sms-kcsd
echo "done."
