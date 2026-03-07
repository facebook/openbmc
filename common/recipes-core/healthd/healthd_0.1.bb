# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Health Monitoring Daemon"
DESCRIPTION = "Daemon for BMC Health monitoring"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://healthd.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

S = "${UNPACKDIR}"
# find -type f | sed 's#^\.#    file:/#' | sed 's#$# \\#' | sort     
LOCAL_URI = " \
    file://healthd.c \
    file://healthd-config.json \
    file://healthd.h \
    file://healthd.service \
    file://meson.build \
    file://plat/meson.build \
    file://README \
    file://run-healthd.sh \
    file://setup-healthd.sh \
"

inherit meson pkgconfig
inherit legacy-packages

pkgdir = "healthd"

inherit systemd

DEPENDS =+ " \
    jansson \
    libkv \
    libmisc-utils \
    libobmc-i2c \
    libpal \
    libvbs \
    libwatchdog \
    update-rc.d-native \
"

install_systemd() {
    install -d ${D}${systemd_system_unitdir}
    install -m 644 ${UNPACKDIR}/healthd/healthd.service ${D}${systemd_system_unitdir}
}

install_sysv() {
    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d
    install -d ${D}${sysconfdir}/sv
    install -d ${D}${sysconfdir}/sv/healthd
    install -d ${D}${sysconfdir}/healthd
    install -m 755 ${UNPACKDIR}/setup-healthd.sh ${D}${sysconfdir}/init.d/setup-healthd.sh
    install -m 755 ${UNPACKDIR}/run-healthd.sh ${D}${sysconfdir}/sv/healthd/run
    update-rc.d -r ${D} setup-healthd.sh start 91 5 .
}

do_install:append() {
    install -d ${D}${sysconfdir}
    install -m 644 ${UNPACKDIR}/healthd-config.json ${D}${sysconfdir}/healthd-config.json

    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        install_systemd
    else
        install_sysv
    fi

}

SYSTEMD_SERVICE:${PN} = "healthd.service"
