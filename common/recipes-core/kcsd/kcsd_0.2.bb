# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "KCS tx/rx daemon"
DESCRIPTION = "The KCS daemon to receive/transmit messages"
SECTION = "base"
PR = "r2"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://kcsd.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://Makefile \
           file://kcsd.c \
           file://setup-kcsd.sh \
           file://kcsd@.service \
           file://kcsd.target \
          "

S = "${WORKDIR}"
DEPENDS += "libipmi libgpio-ctrl liblog update-rc.d-native"

binfiles = "kcsd"

pkgdir = "kcsd"

inherit systemd

install_sysv() {
    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d
    install -m 755 setup-kcsd.sh ${D}${sysconfdir}/init.d/setup-kcsd.sh
    update-rc.d -r ${D} setup-kcsd.sh start 65 5 .
}

install_systemd() {
    install -d ${D}${systemd_system_unitdir}
    install -m 644 kcsd.target ${D}${systemd_system_unitdir}
    install -m 644 "kcsd@.service" ${D}${systemd_system_unitdir}
}

do_install() {
    dst="${D}/usr/local/fbpackages/${pkgdir}"
    bin="${D}/usr/local/bin"
    install -d $dst
    install -d $bin
    install -m 755 kcsd ${dst}/kcsd
    ln -snf ../fbpackages/${pkgdir}/kcsd ${bin}/kcsd

    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        install_systemd
    else
        install_sysv
    fi
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/kcsd ${prefix}/local/bin ${sysconfdir} "

RDEPENDS_${PN} = "libipmi libgpio-ctrl liblog"

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"

SYSTEMD_SERVICE_${PN} = "kcsd@.service kcsd.target"
