FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://mctp-config \
    file://mctpusb-hotplug \
"
RDEPENDS:${PN} += "bash"

do_install:append () {
    install -d ${D}${libexecdir}/mctp

    install -m 0755 ${UNPACKDIR}/mctp-config \
            ${D}${libexecdir}/mctp/

    install -m 0755 ${UNPACKDIR}/mctpusb-hotplug \
            ${D}${libexecdir}/mctp/
}
