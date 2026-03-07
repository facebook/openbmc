SUMMARY = "serfmon emitter"
DESCRIPTION = "Run periodically on the BMC to send serfmon string via the management console"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://serfmon_emitter;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"

inherit systemd

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

S = "${UNPACKDIR}"
SRC_URI += " \
    file://serfmon_emitter \
    file://serfmon_emitter.service \
    file://serfmon_emitter.timer \
"

do_install() {
    install -d ${D}/usr/sbin
    install -d ${D}${systemd_system_unitdir}

    install -m 0755 ${UNPACKDIR}/serfmon_emitter ${D}/usr/sbin
    install -m 0644 ${UNPACKDIR}/serfmon_emitter.service ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/serfmon_emitter.timer ${D}${systemd_system_unitdir}
}

RDEPENDS:${PN} += "python3-core"
FILES:${PN} += "${prefix}/bin ${systemd_system_unitdir}"
SYSTEMD_SERVICE:${PN} = "serfmon_emitter.service serfmon_emitter.timer"
