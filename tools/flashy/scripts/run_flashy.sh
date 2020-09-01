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

# Run this script on an OpenBMC to upgrade via flashy.
set -eo pipefail

# Path to image on OpenBMC
openbmc_image_path="/opt/upgrade/image"
# Path to Flashy on OpenBMC
openbmc_flashy_path="/opt/flashy/flashy"

usage="Usage:
$(basename "$0") --device DEVICE_ID

Run this script on an OpenBMC to upgrade via flashy.

Example DEVICE_ID: \"mtd:flash0\"
Flashy and the image file must already be present in
$openbmc_flashy_path and $openbmc_image_path respectively.
"

# error exit codes for flashy
# defined in OpenBMC tools/flashy/lib/step/error.go
FLASHY_ERROR_SAFE_TO_REBOOT=42
FLASHY_ERROR_UNSAFE_TO_REBOOT=52
handle_flashy_error() {
case $? in
    0)
    return 0
    ;;
    $FLASHY_ERROR_SAFE_TO_REBOOT)
    echo "Flashy exited with safe to reboot error $?" >&2
    exit $?
    ;;
    $FLASHY_ERROR_UNSAFE_TO_REBOOT)
    echo "Flashy exited with unsafe to reboot error $?" >&2
    exit $?
    ;;
    *)
    echo "Unknown flashy exit code $?" >&2
    exit "$?"
    ;;
esac
}

# OpenBMC buildname obtained from /etc/issue
buildname=""
initialize() {
    echo "Getting buildname of device" >&2
    buildname=$(sed -n 's/OpenBMC Release //; s/-.*//p' < /etc/issue)
    echo "Buildname: $buildname" >&2

    echo "Installing flashy" >&2
    /opt/flashy/flashy --install
}

run_checks_and_remediations() {
    echo "Getting common checks and remediations..." >&2
    local common_steps_output=""
    common_steps_output=$(find /opt/flashy/checks_and_remediations/common \
        -type l -maxdepth 1 | sort) \
        || (echo "Error: no common checks and remediations found" >&2 \
        && exit 1)

    local common_steps=()
    readarray -t common_steps <<<"$common_steps_output"

    echo "Common checks and remediations:" >&2
    echo "${common_steps[@]}" | tr ' ' '\n' >&2

    echo "Running common checks and remediations" >&2
    for step in "${common_steps[@]}"
    do
        "$step" --imagepath /opt/upgrade/image --device "$device_id"\
        || handle_flashy_error
    done


    echo "Getting platform-specific checks and remediations..." >&2
    local platform_steps_output=""
    platform_steps_output=$(find /opt/flashy/checks_and_remediations/"$buildname" \
        -type l -maxdepth 1 | sort) \
        || echo "No platform-specific checks and remediations found" >&2 \
        && return 0

    local platform_steps=()
    readarray -t platform_steps <<<"$platform_steps_output"

    echo "Platform-specific checks and remediations:" >&2
    echo "${platform_steps[@]}" | tr ' ' '\n' >&2

    echo "Running platform-specific checks and remediations" >&2
    for step in "${platform_steps[@]}"
    do
        "$step" --imagepath /opt/upgrade/image --device "$device_id" \
        || handle_flashy_error
    done
}

run_flash() {
    echo "Starting to flash..." >&2
    /opt/flashy/flash_procedure/flash_"$buildname" \
    --imagepath "$openbmc_image_path" --device "$device_id" \
    || handle_flashy_error
    echo "Flashing succeeded. It is safe to reboot this system." >&2
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
    *)
    printf "Illegal argument %s\n" "$1" >&2
    exit_with_usage
    ;;
esac
done

[ -n "$device_id" ] || exit_argument_required "device"
[ -f "$openbmc_flashy_path" ] || \
    exit_file_required "Flashy" $openbmc_flashy_path
[ -f "$openbmc_image_path" ] || \
    exit_file_required "Image" $openbmc_image_path

initialize
run_checks_and_remediations
run_flash
