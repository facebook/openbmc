SUMMARY = "Header only library for iso-week"
DESCRIPTION = "Header only library for iso-week"
HOMEPAGE = "https://github.com/HowardHinnant/date"
SECTION = "libs/devel"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=b5d973344b3c7bbf7535f0e6e002d017"

PV = "3.0.1"
SRC_URI = "https://github.com/HowardHinnant/date/archive/refs/tags/v${PV}.tar.gz"
SRC_URI[md5sum] = "78902f47f7931a3ae8a320e0dea1f20a"
SRC_URI[sha256sum] = "7a390f200f0ccd207e8cff6757e04817c1a0aec3e327b006b7eb451c57ee3538"

S = "${WORKDIR}/date-${PV}"

FILES:${PN}-dev := " \
    ${includedir} \
"

do_compile() {
    :
}
do_install() {
    install -d ${D}/${includedir}/date
    install -m 755 -t ${D}/${includedir}/date ${S}/include/date/*
}
