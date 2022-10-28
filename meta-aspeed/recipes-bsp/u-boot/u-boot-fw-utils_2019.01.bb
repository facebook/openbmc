# Copyright 2019-present Facebook. All Rights Reserved.
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

SUMMARY = "U-Boot bootloader fw_printenv/setenv utilities"
LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://Licenses/README;md5=30503fd321432fc713238f582193b78e"
SECTION = "bootloader"
DEPENDS = "mtd-utils bison-native"

SRCBRANCH = "openbmc/helium/v2019.01"
SRCREV = "AUTOINC"

SRC_URI = "git://github.com/facebook/openbmc-uboot.git;branch=${SRCBRANCH};protocol=https \
           file://fw_env.config \
          "

PV = "v2019.01+git${SRCPV}"
S = "${WORKDIR}/git"
include common/recipes-bsp/u-boot-fbobmc/use-intree-shipit.inc

INSANE_SKIP:${PN} = "already-stripped"
EXTRA_OEMAKE:class-target = 'CROSS_COMPILE=${TARGET_PREFIX} CC="${CC} ${CFLAGS} ${LDFLAGS}" HOSTCC="${BUILD_CC} ${BUILD_CFLAGS} ${BUILD_LDFLAGS}" V=1'
EXTRA_OEMAKE:class-cross = 'ARCH=${TARGET_ARCH} CC="${CC} ${CFLAGS} ${LDFLAGS}" V=1'

inherit uboot-config
# default fw_env config file to be installed
FW_ENV_CONFIG_FILE ??= "fw_env.config"

do_compile () {
  oe_runmake -C ${S} ${UBOOT_MACHINE} O=${B}
  oe_runmake -C ${S} envtools O=${B}
}

do_install () {
  install -d ${D}${base_sbindir}
  install -d ${D}${sysconfdir}
  install -m 755 ${B}/tools/env/fw_printenv ${D}${base_sbindir}/fw_printenv
  install -m 755 ${B}/tools/env/fw_printenv ${D}${base_sbindir}/fw_setenv
  bbdebug 1 "install ${FW_ENV_CONFIG_FILE}"
  install -m 0644 ${WORKDIR}/${FW_ENV_CONFIG_FILE} ${D}${sysconfdir}/fw_env.config
}

do_install:class-cross () {
  install -d ${D}${bindir_cross}
  install -m 755 ${B}/tools/env/fw_printenv ${D}${bindir_cross}/fw_printenv
  install -m 755 ${B}/tools/env/fw_printenv ${D}${bindir_cross}/fw_setenv
}

SYSROOT_DIRS:append:class-cross = " ${bindir_cross}"

PACKAGE_ARCH = "${MACHINE_ARCH}"
BBCLASSEXTEND = "cross"

DEFAULT_PREFERENCE = "-1"
