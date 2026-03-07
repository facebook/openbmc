LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit allarch systemd

RDEPENDS:${PN} += "bash"

S = "${UNPACKDIR}"
SRC_URI += " \
    file://nic-reset@.service \
    file://nic-reset.sh \
    "

SYSTEMD_SERVICE:${PN}:append = " \
    nic-reset@.service   \
    "

do_install:append() {
    install -d ${D}${libexecdir}
    install -m 0755 ${UNPACKDIR}/nic-reset.sh ${D}${libexecdir}/nic-reset

    install -d ${D}/${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/nic-reset@.service ${D}/${systemd_system_unitdir}/nic-reset@.service
}

pkg_postinst:${PN} () {
    NIC_INSTANCES="${@d.getVar('OBMC_NIC_INSTANCES', True)}"
    for i in ${NIC_INSTANCES};
    do
         ln -s ./nic-reset@.service $D$systemd_system_unitdir/nic-reset@${i}.service
    done;
}
