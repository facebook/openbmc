DESCRIPTION = "flashrom is a utility for identifying, reading, writing, verifying and erasing flash chips"
LICENSE = "GPLv2"
HOMEPAGE = "http://flashrom.org"

LIC_FILES_CHKSUM = "file://COPYING;md5=751419260aa954499f7abaabaa882bbe"
DEPENDS = "pciutils"

SRC_URI = "https://download.flashrom.org/releases/flashrom-${PV}.tar.bz2 \
           file://flashrom-${PV}/make.local \
           file://01-include-make-local.patch \
          "

Src_URI[md5sum] = "ac513076b63ab7eb411a7694bb8f6fda"
SRC_URI[sha256sum] = "13dc7c895e583111ecca370363a3527d237d178a134a94b20db7df177c05f934"

do_install() {
    oe_runmake PREFIX=${prefix} DESTDIR=${D} install
}
