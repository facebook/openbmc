# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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
#

include linux-aspeed.inc
require recipes-kernel/linux/linux-yocto.inc

KBRANCH = "dev-6.1"
KSRC ?= "git://github.com/openbmc/linux;protocol=https;branch=${KBRANCH}"
SRC_URI += "${KSRC}"

SRCREV = "cac6306557586ba4479dab12c3e5efcb67e63388"
LINUX_VERSION ?= "6.1.12"
LINUX_VERSION_EXTENSION ?= "-aspeed"
PR = "r1"
PV = "${LINUX_VERSION}"
LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"

#
# Include common defconfig file
#
SRC_URI += "file://defconfig-6.1/${SOC_MODEL}/defconfig"

#
# Apply JFFS2 workarounds recommended for all the OpenBMC platforms
# with JFFS2 partition.
#
SRC_URI += "file://patches/0101-jffs2-kill-garbage-collect-thread-when-filesystem-is.patch \
            file://patches/0102-jffs2-suppress-jffs2-messages-when-reading-inode.patch \
           "

#
# Apply eMMC workarounds recommended for all the OpenBMC platforms with
# eMMC.
#
SRC_URI += "file://patches/0105-mmc-sdhci-of-aspeed-add-skip_probe-module-parameter.patch \
           "

#
# Apply spi-aspeed-user driver patch to access non-flash SPI devices, such
# as EEPROMs.
# The patch should be dropped once the user-mode logic is added to
# spi-aspeed-smc driver.
#
SRC_URI += "file://patches/0141-spi-add-user-mode-aspeed-spi-driver.patch"

do_kernel_configme[depends] += "virtual/${TARGET_PREFIX}gcc:do_populate_sysroot"
KCONFIG_MODE="--alldefconfig"

S = "${WORKDIR}/git"

#
# Note: below fixup is needed to bitbake linux kernel 5.2 or higher kernel
# versions using yocto-rocko. It's usually needed by ast2400 BMC platforms
# because they have to stay with u-boot-v2016.07 which cannot be compiled
# by newer (than rocko) yocto versions.
#
python () {
    if d.getVar('DISTRO_CODENAME') == 'rocko':
        d.appendVar(
            'FILES:kernel-base',
            ' ${nonarch_base_libdir}/modules/${KERNEL_VERSION}/modules.builtin.modinfo')
}
