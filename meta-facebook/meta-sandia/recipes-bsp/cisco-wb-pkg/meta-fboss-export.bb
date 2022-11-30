# Copyright (c) 2022 by Cisco Systems, Inc.
# All rights reserved.
#
SUMMARY = "cisco-open/meta-fboss-export"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://${S}/LICENSE;md5=3b83ef96387f14655fc854ddc3c6bd57"

PR = "r0"
PV = "0.16-1"

inherit cmake

SRCREV = "01f91b13a2a3e4a0daea120c25920f862754adac"
SRC_URI = " \
    git://git@github.com:/cisco-open/meta-fboss-export.git;branch=main;protocol=ssh \
"

RREPLACES:${PN} += "wedge-eeprom"
RCONFLICTS:${PN} += "wedge-eeprom"

RDEPENDS:${PN} += " \
    bash \
"

DEPENDS += " \
    gflags \
    glog \
    howardhinnant-date \
    nlohmann-json \
    zlib \
"

S = "${WORKDIR}/git"

do_patch() {
    rm -f git/cmake/rackmon.cmake

    cp git/fboss/platform/weutil/bmc/* \
        git/fboss/platform/weutil/
    cp git/fboss/platform/fw_util/bmc/* \
        git/fboss/platform/fw_util/

    sed -i -e 's| opt/cisco/bin$| ${CMAKE_INSTALL_BINDIR}|' \
        git/cmake/fw_util.cmake
}

FILES:${PN} := " \
    ${sysconfdir}/udev/rules.d/9* \
    ${bindir} \
    /opt/cisco/etc/fboss \
    /opt/cisco/etc/metadata \
    /opt/cisco/bin \
"

do_install() {
    install -d ${D}${sysconfdir}/udev/rules.d
    install -m 644 -t ${D}${sysconfdir}/udev/rules.d \
        ${S}/target/sandia/etc/udev/rules.d/99-bmc-85_SCM_O_BMC.rules

    install -d ${D}${bindir}
    install -m 755 ${WORKDIR}/build/fw_util_cisco ${D}${bindir}/fw_util
    install -m 755 ${WORKDIR}/build/weutil_cisco  ${D}${bindir}/weutil

    install -d ${D}/opt/cisco/bin/
    install -m 755 -t ${D}/opt/cisco/bin ${S}/fpd/bios_bmc_create_mtd.sh
    install -m 755 -t ${D}/opt/cisco/bin ${S}/fpd/bios_bmc_helper.sh
    install -m 755 -t ${D}/opt/cisco/bin ${S}/fpd/bios_mtd_upgrade.sh
    install -m 755 -t ${D}/opt/cisco/bin ${S}/fpd/bios_bmc_select_mux.sh
    install -m 755 -t ${D}/opt/cisco/bin ${S}/fpd/bios_bmc_unselect_mux.sh
}
