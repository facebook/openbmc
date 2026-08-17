SUMMARY = "SSIF IPMI Bridge Daemon"
DESCRIPTION = "Daemon that bridges SSIF host interface to ipmid"
SECTION = "base"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

S = "${UNPACKDIR}"
LOCAL_URI = " \
    file://meson.build \
    file://check_ssifd.sh \
    file://ssifd.c \
    file://ssifd.service \
    "

DEPENDS += " \
    libipmi \
    liblog \
    "

RDEPENDS:${PN} += " \
    libipmi \
    liblog \
    "

inherit meson systemd

pkgdir = "ssifd"

do_install:append() {
    install -d ${D}${prefix}/local/bin
    install -m 0755 ${S}/check_ssifd.sh ${D}${prefix}/local/bin/check_ssifd.sh
    ln -snf ../fbpackages/${pkgdir}/ssifd ${D}${prefix}/local/bin/ssifd

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/ssifd.service ${D}${systemd_system_unitdir}/ssifd.service
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"
FILES:${PN} = "${FBPACKAGEDIR}/ssifd ${prefix}/local/bin ${sysconfdir} "

SYSTEMD_SERVICE:${PN} = "ssifd.service"
