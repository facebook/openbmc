LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit allarch systemd obmc-phosphor-systemd

RDEPENDS:${PN} += "bash"

S = "${UNPACKDIR}"
SRC_URI += " \
    file://sensor-cache.timer \
    file://sensor-cache.service \
    file://sensor-cache.sh \
    "

SYSTEMD_SERVICE:${PN}:append = " \
    sensor-cache.service \
    sensor-cache.timer \
    "

do_install() {
    install -d ${D}/${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/sensor-cache.service ${D}/${systemd_system_unitdir}/sensor-cache.service
    install -m 0644 ${UNPACKDIR}/sensor-cache.timer ${D}/${systemd_system_unitdir}/sensor-cache.timer
    install -d ${D}${libexecdir}
    install -m 0755 ${UNPACKDIR}/sensor-cache.sh ${D}${libexecdir}/sensor-cache
}
