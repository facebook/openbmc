FILESEXTRAPATHS:append := "${THISDIR}/files:"

SUMMARY = "OpenBMC BIC Update Tool"
DESCRIPTION = "OpenBMC BIC Update Tool"

PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

S = "${WORKDIR}"

SRC_URI = " \
    file://bic-update.sh \
    "

RDEPENDS:${PN} = "bash"

do_install() {
    install -d ${D}/${bindir}
    install -m 0755 ${S}/bic-update.sh ${D}/${bindir}/
}

