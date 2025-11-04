LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit allarch systemd obmc-phosphor-systemd

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

RDEPENDS:${PN} += "bash managed-reboot"

SRC_URI += " \
    file://usb-serial-monitor.sh \
    file://usb-serial-monitor.service \
    "

SYSTEMD_SERVICE:${PN}:append = " \
    usb-serial-monitor.service \
    usb-serial-monitor.timer \
    "

do_install() {
    install -d ${D}/${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/usb-serial-monitor.service ${D}/${systemd_system_unitdir}/usb-serial-monitor.service
    install -m 0644 ${UNPACKDIR}/usb-serial-monitor.timer ${D}/${systemd_system_unitdir}/usb-serial-monitor.timer
    install -d ${D}${libexecdir}
    install -m 0755 ${UNPACKDIR}/usb-serial-monitor.sh ${D}${libexecdir}/usb-serial-monitor
}
