FILESEXTRAPATHS:append := "${THISDIR}/files:"

SUMMARY = "OpenBMC PLDM Update Tool"
DESCRIPTION = "OpenBMC PLDM Update Tool"

PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

LOCAL_URI = " \
    file://pldm-fw-update.sh \
    file://uart_fw_py_arm32  \
    file://ast1030_uart_fw.bin  \
    "

RDEPENDS:${PN} = "bash"
RDEPENDS:${PN} += "zlib"
INSANE_SKIP:${PN} += "already-stripped"
do_install() {
    install -d ${D}/${bindir}
    install -m 0755 ${UNPACKDIR}/pldm-fw-update.sh ${D}/${bindir}/
    install -m 0777 ${UNPACKDIR}/uart_fw_py_arm32  ${D}/${bindir}/
    install -m 0755 ${UNPACKDIR}/ast1030_uart_fw.bin ${D}/${bindir}/
}

