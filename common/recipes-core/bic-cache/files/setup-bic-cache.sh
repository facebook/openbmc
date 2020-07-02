#!/bin/bash
#
# Copyright 2018-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-bic-cache
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set Cachcing for Bridge IC info
### END INIT INFO
# shellcheck disable=SC1091
source /usr/local/fbpackages/utils/ast-functions

echo -n "Setup Caching for Bridge IC info.."

/usr/local/bin/bic-cache 0 > /dev/null 2>&1 &

echo "done."
