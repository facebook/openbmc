SUMMARY = "FTDI MDIO Tool"
DESCRIPTION = "FTDI utility that uses libftdi to perform MDIO communication."

PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit meson pkgconfig systemd
DEPENDS = "libusb1 libftdi"

FILES:${PN} += " \
    ${systemd_system_unitdir} \
    "

S = "${UNPACKDIR}"
SRC_URI = "file://ftdi_mdio_cli.c \
           file://ftdi_mdio_core.c \
           file://ftdi_mdio_core.h \
           file://ftdi_mdio_daemon.c \
           file://ftdi_mdio_daemon.h \
           file://meson.build \
           file://marvell-switch-init_evt \
           file://marvell-switch-init_evt.service \
           file://99-ftdi-mdio.rules \
           file://systemd-networkd.service.d/after-marvell-switch-init_evt.conf \
          "

SYSTEMD_SERVICE:${PN} = " \
    marvell-switch-init_evt.service \
    "

RDEPENDS:${PN} += "bash"

do_install:append() {
    LIBEXECDIR_PN="${D}${libexecdir}/${PN}"
    install -d ${LIBEXECDIR_PN}
    install -m 0755 ${UNPACKDIR}/marvell-switch-init_evt ${LIBEXECDIR_PN}

    install -d ${D}${sysconfdir}/udev/rules.d
    install -m 0644 ${UNPACKDIR}/99-ftdi-mdio.rules ${D}${sysconfdir}/udev/rules.d

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/marvell-switch-init_evt.service ${D}${systemd_system_unitdir}

    install -d ${D}${systemd_system_unitdir}/systemd-networkd.service.d
    install -m 0644 ${UNPACKDIR}/systemd-networkd.service.d/after-marvell-switch-init_evt.conf \
            ${D}${systemd_system_unitdir}/systemd-networkd.service.d/
}

