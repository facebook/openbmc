DESCRIPTION = "flashrom is a utility for identifying, reading, writing, verifying and erasing flash chips"
LICENSE = "GPL-2.0-or-later"
HOMEPAGE = "http://flashrom.org"

DEPENDS = "pciutils libusb libusb-compat"

LIC_FILES_CHKSUM = "file://COPYING;md5=751419260aa954499f7abaabaa882bbe"
SRC_URI = "https://download.flashrom.org/releases/flashrom-v${PV}.tar.bz2 \
           file://0001-flashrom-GD25LQ256D.patch \
           file://0002-flashrom-remove-duplicate-N25Q256__1E.patch \
           file://0003-flashrom-W25Q256JW.patch \
           "

SRC_URI[md5sum] = "7f8e4b87087eb12ecee0fcc5445b4956"
SRC_URI[sha256sum] = "e1f8d95881f5a4365dfe58776ce821dfcee0f138f75d0f44f8a3cd032d9ea42b"
S = "${WORKDIR}/flashrom-v${PV}"

inherit pkgconfig

do_install() {
    oe_runmake PREFIX=${prefix} DESTDIR=${D} install
}
