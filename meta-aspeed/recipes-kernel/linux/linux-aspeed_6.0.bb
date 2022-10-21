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

KBRANCH = "dev-6.0"
KSRC ?= "git://github.com/openbmc/linux;protocol=https;branch=${KBRANCH}"
SRC_URI += "${KSRC}"

SRCREV = "b380fe3037afd47726ab84684df5ee620793bb25"
LINUX_VERSION ?= "6.0.2"
LINUX_VERSION_EXTENSION ?= "-aspeed"
PR = "r1"
PV = "${LINUX_VERSION}"
LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"

#
# Include common defconfig file
#
SRC_URI += "file://defconfig-6.0/${SOC_MODEL}/defconfig"

#
# Backport a few patches from upstream
#
SRC_URI += "file://patches/0001-net-ftgmac100-support-fixed-link.patch \
            file://patches/0002-ARM-dts-aspeed-elbert-Enable-mac3-controller.patch \
           "

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
# Apply JTAG patches required for all the OpenBMC platforms with JTAG
# enabled.
#
# The patches are derived from below patch series, and all the additional
# JTAG patches in "linux-aspeed-5.15" tree are also included.
# https://lwn.net/Articles/817430/
#
# There won't be merge conflicts for the first 7 patches [111-117]: all
# are new files.
# Please prepare to resolve conflicts when applying patches [118-122]
# to different kernel versions, but it should be straightforward.
#
SRC_URI += "file://patches/0111-drivers-jtag-Add-JTAG-core-driver.patch \
            file://patches/0112-dt-binding-jtag-Aspeed-2400-and-2500-series.patch \
            file://patches/0113-Add-Aspeed-SoC-24xx-and-25xx-families-JTAG-master-dr.patch \
            file://patches/0114-jtag-aspeed-support-sysfs-interface.patch \
            file://patches/0115-jtag-support-driver-specific-ioctl-commands.patch \
            file://patches/0116-jtag-aspeed-support-JTAG_RUN_CYCLE-ioctl-command.patch \
            file://patches/0117-jtag-aspeed-add-AST2600-support.patch \
            file://patches/0118-Documentation-jtag-Add-ABI-documentation.patch \
            file://patches/0119-Documentation-jtag-Add-JTAG-core-driver-ioctl-number.patch \
            file://patches/0120-drivers-jtag-Add-JTAG-core-driver-Maintainers.patch \
            file://patches/0121-fixup-jtag-patch-series.patch \
            file://patches/0122-ARM-dts-aspeed-g6-add-jtag-controllers.patch \
            file://patches/0123-ARM-dts-aspeed-g5-add-jtag-controller.patch \
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
