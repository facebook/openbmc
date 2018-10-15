# Copyright 2018-present Facebook. All Rights Reserved.
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
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

# Run a command given as the second through nth arguments, and report whether
# it succeeded in a pretty fashion to both stdout and syslog, using the action
# message given in the first argument. Examples of good action messages are
# "enabling userver power" and "starting network service".
#
# Note that the command should not produce output of its own, or else the
# stdout message's formatting will be disrupted.
try_and_log() {
    local message ret

    message="$1"; shift
    echo -n "$message ..."
    logger "$message"

    "$@"
    ret="$?"
    if [ "$ret" -eq 0 ]; then
        echo " done"
        logger "finished $message"
    else
        echo " failed"
        logger "failed while $message"
    fi

    return "$ret"
}
