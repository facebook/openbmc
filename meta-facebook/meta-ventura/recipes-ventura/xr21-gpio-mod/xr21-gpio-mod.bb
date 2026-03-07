SUMMARY = "XR21B1424 GPIO MOD"
DESCRIPTION = "With the CDC_ACM driver, MaxLinear provided a utility to set the XR21B1424's GPIO mode to GPIO5 to match the circuit design"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit systemd

RDEPENDS:${PN} += "python3-pyusb python3-misc"

FILES:${PN} += " \
    ${libexecdir}/ventura \
    ${systemd_system_unitdir} \
    "

S = "${UNPACKDIR}"
SRC_URI = " \
    file://gpio5_auto_rs485.py \
    file://xr21-gpio-mod.service \
    "

SYSTEMD_SERVICE:${PN} = " \
    xr21-gpio-mod.service \
    "

do_install() {
    VENTURA_LIBEXECDIR="${D}${libexecdir}/ventura"
    install -d ${VENTURA_LIBEXECDIR}
    install -m 0755 ${UNPACKDIR}/gpio5_auto_rs485.py ${VENTURA_LIBEXECDIR}

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/xr21-gpio-mod.service ${D}${systemd_system_unitdir}
}
