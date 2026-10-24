FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://mctp-config \
    file://mctpusb-hotplug \
    file://mctpusb-hotplug@.service \
    file://mctpusb-init \
    file://mctpusb-init.service \
"
RDEPENDS:${PN} += "bash mctp"

FILES:${PN}:append = " \
    ${libexecdir}/mctp \
"

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE:${PN}:append = " \
    mctpusb-hotplug@.service \
    mctpusb-init.service \
"

do_install:append () {
    install -d ${D}${libexecdir}/mctp

    install -m 0755 ${UNPACKDIR}/mctp-config \
            ${D}${libexecdir}/mctp/

    install -m 0755 ${UNPACKDIR}/mctpusb-hotplug \
            ${D}${libexecdir}/mctp/

    install -m 0755 ${UNPACKDIR}/mctpusb-init \
            ${D}${libexecdir}/mctp/

    install -d ${D}${systemd_system_unitdir}

    install -m 0644 ${UNPACKDIR}/mctpusb-hotplug@.service \
            ${D}${systemd_system_unitdir}

    install -m 0644 ${UNPACKDIR}/mctpusb-init.service \
            ${D}${systemd_system_unitdir}
}
