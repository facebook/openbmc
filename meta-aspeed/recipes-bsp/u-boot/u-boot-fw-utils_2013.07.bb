SUMMARY = "U-Boot bootloader fw_printenv/setenv utilities"
LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://COPYING;md5=1707d6db1d42237583f50183a5651ecb"
SECTION = "bootloader"
DEPENDS = "mtd-utils"

# This revision corresponds to the tag "v2013.07"
# We use the revision in order to avoid having to fetch it from the
# repo during parse
SRCREV = "cf1d5829e3aa8c0b8f8eac5fa818f9e5aca09ae2"

PV = "v2013.07-openbmc"

SRC_URI = "git://github.com/theopolis/u-boot-openbmc.git;branch=${PV};protocol=git \
          "

S = "${WORKDIR}/git"

EXTRA_OEMAKE = 'HOSTCC="${CC}" HOSTSTRIP="true"'

inherit uboot-config

do_compile () {
  oe_runmake ${UBOOT_MACHINE}
  oe_runmake env
}

do_install () {
  install -d ${D}${base_sbindir}
  install -m 755 ${S}/tools/env/fw_printenv ${D}${base_sbindir}/fw_printenv
  install -m 755 ${S}/tools/env/fw_printenv ${D}${base_sbindir}/fw_setenv
}

PACKAGE_ARCH = "${MACHINE_ARCH}"
