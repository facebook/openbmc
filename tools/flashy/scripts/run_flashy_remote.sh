#!/bin/bash
# Copyright 2020-present Facebook. All Rights Reserved.
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

# Run this script to upgrade a remote OpenBMC.
set -eo pipefail
# make sure we're in flashy's project root
cd "$(dirname "$0")" && cd ..

# Path to image on OpenBMC
openbmc_image_path="/opt/upgrade/image"
# Path to Flashy on OpenBMC
openbmc_flashy_path="/opt/flashy/flashy"
# Path to run_flashy upgrade script from project root
run_flashy_script_path="scripts/run_flashy.sh"

options=(-o "LogLevel=error" -o "StrictHostKeyChecking=no")
options+=(-o "UserKnownHostsFile=/dev/null")
sshpassCmd=(sshpass -p0penBmc)
sshcmd=("${sshpassCmd[@]}" ssh "${options[@]}")

usage="Usage:
$(basename "$0") --device DEVICE_ID --imagepath IMAGE_PATH --host OPENBMC_HOSTNAME

Run this script to upgrade a remote OpenBMC.

Example DEVICE_ID: \"mtd:flash0\"
"

scpfile() {
    file=${1:?Please specify file to scp}
    dst=${2:?Please specify destination directory}
    local target=""
    target=$host

    if [[ "$target" =~ .*:.* ]]
    then
        target="[$host]"
    fi

    "${sshpassCmd[@]}" scp "${options[@]}" "$file" "root@$target:$dst"
}

initialize() {
    echo "Building flashy..." >&2
    ./build.sh

    echo "Making installation directories on OpenBMC..." >&2
    "${sshcmd[@]}" "mkdir -p /opt/flashy /opt/upgrade"

    echo "Copying flashy..." >&2
    scpfile ./build/flashy "$openbmc_flashy_path"

    echo "Copying image..." >&2
    scpfile "$imagepath" "$openbmc_image_path"

    echo "Copying upgrade script..." >&2
    scpfile "$run_flashy_script_path" "/tmp"
}

run_upgrade() {
    echo "Running upgrade..." >&2
    "${sshcmd[@]}" "/tmp/run_flashy.sh --device $device_id"
}

reboot_and_check_upgrade() {
    echo "Flashing complete, rebooting the device..." >&2
    "${sshcmd[@]}" "reboot"

    check_cmd="head -n 1 /etc/issue; uptime"

    while true
    do
        echo "Waiting for reboot..." >&2
        sleep 15
        "${sshcmd[@]}" -o ConnectTimeout=15 "$check_cmd" && break
    done
    echo "Upgrade done!" >&2
}

exit_with_usage() {
    echo -e "$usage" >&2
    exit 1
}

exit_argument_required() {
    echo "$1 argument is required" >&2
    exit_with_usage
}

exit_file_required() {
    echo "$1 not found in '$2'" >&2
    exit_with_usage
}

# Flash device ID to flash (e.g. mtd:flash0)
device_id=""
# Path to OpenBMC image on local system
# i.e. what we scp to $host as "/opt/upgrade/image"
imagepath=""
# OpenBMC hostname
host=""
while [[ $# -gt 0 ]]
do
key="$1"
[ -n "$2" ] || exit_argument_required "$1"
case $key in
    -h|-help|--help)
    echo -e "$usage"
    exit 0
    ;;
    --device)
    device_id="$2"
    shift
    shift
    ;;
    --imagepath)
    imagepath="$2"
    shift
    shift
    ;;
    --host)
    host="$2"
    shift
    shift
    ;;
    *)
    printf "Illegal argument %s\n" "$1" >&2
    exit_with_usage
    ;;
esac
done

confirm_continue() {
    read -rp "Continue (y/n)?" choice
    case "$choice" in
    y|Y)
    ;;
    *)
    echo "Exiting"
    exit 1
    ;;
    esac
}

[ -n "$device_id" ] || exit_argument_required "device"
[ -n "$host" ] || exit_argument_required "host"
[ -n "$imagepath" ] || exit_argument_required "imagepath"
[ -f "$imagepath" ] || exit_file_required "Image" "$imagepath"

sshcmd+=("root@$host")

echo "Running a remote upgrade on '$host' with image '$imagepath'"
if [ -t 0 ]
then
    confirm_continue
fi

initialize
run_upgrade
reboot_and_check_upgrade
