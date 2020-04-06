#!/bin/bash
# Selects the primary chip and re-flashes it with the image URL given
# as an argument, or with the default one from yum.

set -xeuo pipefail

platform=$(awk '{print $NF}' /etc/issue|cut -d- -f1)

function default_url() {
    echo "http://yum/fbpkg/openbmc.image.${platform}:DEFAULT"
}

url=${1:-$(default_url)}

if [ "${platform}" = "wedge100" ]
then
    openbmc_gpio_util.py config ROMCS1#
fi

devmem 0x1e785034 32 0x1

dev=$(awk -F: '/"flash0"/{print $1}' /proc/mtd)

wget "$url" -O image

flashcp image "/dev/${dev}"

echo "All flashed to a safe version! Now reboot"
