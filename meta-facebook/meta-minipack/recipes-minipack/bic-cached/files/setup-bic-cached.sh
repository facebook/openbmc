#!/bin/sh
#
# Copyright 2018-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-bic-cached
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set Cachcing for Bridge IC info
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

echo -n "Setup Caching for Bridge IC info.."

/usr/local/bin/bic-cached 0 > /dev/null 2>&1 &

echo "done."
