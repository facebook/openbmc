#!/bin/sh
#
# Copyright 2024-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#

### BEGIN INIT INFO
# Provides:          setup-eth0-ncsi-fixup
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Start eth0 NCSI fixup daemon
### END INIT INFO

echo -n "Setup eth0 NCSI fixup daemon.."
runsv /etc/sv/eth0-ncsi-fixup > /dev/null 2>&1 &
echo "done."
