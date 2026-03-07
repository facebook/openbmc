SUMMARY = "Helper tool to add overlay for common partitions"
DESCRIPTION = "A helper utility to create overlay partitions"
SECTION = "base"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

S = "${UNPACKDIR}"
SRC_URI += " \
    file://overlay-helper.sh \
    "

RDEPENDS:${PN} += " \
    bash \
    "

do_install() {
    install -d ${D}/${bindir}
    install -m 755 ${UNPACKDIR}/overlay-helper.sh ${D}${bindir}/overlay-helper
}
