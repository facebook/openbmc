SUMMARY = "OpenBMC CXL Coredump Collection Tool"
DESCRIPTION = "OpenBMC CXL Coredump Collection Tool"

PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

RDEPENDS:${PN} += "bash expect"

S = "${UNPACKDIR}"
SRC_URI += " \
    file://cxl-coredump-collection.expect \
    "
do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${UNPACKDIR}/cxl-coredump-collection.expect ${D}/${bindir}/cxl-coredump-collection
}
