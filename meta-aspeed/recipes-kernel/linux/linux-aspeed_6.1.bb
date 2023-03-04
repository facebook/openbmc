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
include linux-patches-6.1.inc

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
