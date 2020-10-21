FILESEXTRAPATHS_append := "${THISDIR}/${PN}:"

SRC_URI += " \
            file://logrotate-3.9.1/examples/logrotate-default \
            file://logrotate-3.9.1/examples/wtmp_default \
           "

do_install_append() {
    install -p -m 644 ${WORKDIR}/logrotate-3.9.1/examples/logrotate-default ${D}${sysconfdir}/logrotate.conf
    install -p -m 644 ${WORKDIR}/logrotate-3.9.1/examples/wtmp_default ${D}${sysconfdir}/logrotate.d/wtmp
}
