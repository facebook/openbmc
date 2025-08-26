SUMMARY = "eth0-mac-fixup"
DESCRIPTION = "Fix BMC eth0 MAC address"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://eth0_mac_fixup.sh;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

S="${WORKDIR}/sources"
UNPACKDIR="${S}"

LOCAL_URI = " \
    file://eth0_mac_fixup.sh \
    "

do_install() {
    install -d ${D}/usr/local/bin

    install -m 0755 ${UNPACKDIR}/eth0_mac_fixup.sh ${D}/usr/local/bin
}

RDEPENDS:${PN} += "bash"
FILES:${PN} += "${prefix}/local/bin ${sysconfdir} "
