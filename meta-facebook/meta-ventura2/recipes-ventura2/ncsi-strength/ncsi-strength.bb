SUMMARY = "NCSI Strength Tool"
DESCRIPTION = "Modify the AST2600 NCSI strength."

PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit meson pkgconfig systemd

FILES:${PN} += " \
    ${libexecdir}/ventura2 \
    ${systemd_system_unitdir} \
    "

S = "${UNPACKDIR}"
SRC_URI = "file://ncsi-strength.c \
           file://meson.build \
           file://ncsi-strength.sh \
           file://ncsi-strength.service \
          "

SYSTEMD_SERVICE:${PN} = " \
    ncsi-strength.service \
    "

SYSTEMD_AUTO_ENABLE = "enable"

SYSTEMD_SERVICE:${PN}:append = " \                                              
    ncsi-strength.service   \                                                      
    "

DEPENDS += "libphymem"
RDEPENDS:${PN} += "bash"

do_install:append() {
    VENTURA2_LIBEXECDIR="${D}${libexecdir}/${PN}"
    install -d ${VENTURA2_LIBEXECDIR}
    install -m 0755 ${UNPACKDIR}/ncsi-strength.sh ${VENTURA2_LIBEXECDIR}

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/ncsi-strength.service ${D}${systemd_system_unitdir}

    install -d ${D}${bindir}
    install -m 0755 ${B}/ncsi-strength ${D}${bindir}/ncsi-strength
}

