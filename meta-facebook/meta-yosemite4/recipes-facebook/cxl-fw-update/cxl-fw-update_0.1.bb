SUMMARY = "OpenBMC CXL Update Tool"
DESCRIPTION = "OpenBMC CXL Update Tool"

PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

S = "${WORKDIR}"

inherit meson pkgconfig

LOCAL_URI = " \
    file://cxl-update.cpp \
    file://cxl-update.hpp \
    file://cci-mctp-update.cpp \
    file://cci-mctp-update.hpp \
    file://meson.build \
    file://cxl-fw-update.sh \
    "

DEPENDS += " \
    cli11 \
    sdbusplus \
    "

RDEPENDS:${PN} = "bash"

do_install:append() {
    install -d ${D}/${bindir}
    install -m 0755 ${S}/cxl-fw-update.sh ${D}/${bindir}/
}
