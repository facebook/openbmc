SUMMARY = "eth0-mac-fixup"
DESCRIPTION = "Fix BMC eth0 MAC address"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://eth0_mac_fixup.sh;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"


inherit systemd

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

S="${WORKDIR}/sources"
UNPACKDIR="${S}"

LOCAL_URI = " \
    file://eth0_mac_fixup.sh \
    file://eth0-mac-fixup.service \
    "

do_install() {
    install -d ${D}/usr/bin
    install -d ${D}${systemd_system_unitdir}

    install -m 0755 ${UNPACKDIR}/eth0_mac_fixup.sh ${D}/usr/bin
    install -m 0644 ${UNPACKDIR}/eth0-mac-fixup.service ${D}${systemd_system_unitdir}
}

RDEPENDS:${PN} += "bash"
FILES:${PN} += "${prefix}/bin ${sysconfdir} "
SYSTEMD_SERVICE:${PN} = "eth0-mac-fixup.service"
