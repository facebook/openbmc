#!/bin/sh
#
# Copyright 2014-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-rest-api
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set REST API handler
### END INIT INFO

echo -n "Setup REST API handler... "
/usr/local/bin/rest.py > /tmp/rest.log 2>&1 &
echo "done."
