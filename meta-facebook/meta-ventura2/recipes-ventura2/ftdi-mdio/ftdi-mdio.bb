SUMMARY = "FTDI MDIO Tool"
DESCRIPTION = "FTDI utility that uses libftdi to perform MDIO communication."

PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit meson pkgconfig systemd
DEPENDS = "libusb1 libftdi"

S = "${UNPACKDIR}"
SRC_URI = "file://ftdi_mdio_cli.c \
           file://ftdi_mdio_core.c \
           file://ftdi_mdio_core.h \
           file://ftdi_mdio_daemon.c \
           file://ftdi_mdio_daemon.h \
           file://meson.build \
          "
