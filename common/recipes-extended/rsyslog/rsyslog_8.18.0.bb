SUMMARY = "Rsyslog is an enhanced multi-threaded syslogd"
DESCRIPTION = "\
Rsyslog is an enhanced syslogd supporting, among others, MySQL,\
 PostgreSQL, failover log destinations, syslog/tcp, fine grain\
 output format control, high precision timestamps, queued operations\
 and the ability to filter on any message part. It is quite\
 compatible to stock sysklogd and can be used as a drop-in replacement.\
 Its advanced features make it suitable for enterprise-class,\
 encryption protected syslog relay chains while at the same time being\
 very easy to setup for the novice user."

DEPENDS = "zlib libestr libfastjson liblogging bison-native flex-native"
HOMEPAGE = "http://www.rsyslog.com/"
LICENSE = "GPLv3 & LGPLv3 & Apache-2.0"
LIC_FILES_CHKSUM = "file://COPYING;md5=51d9635e646fb75e1b74c074f788e973 \
                    file://COPYING.LESSER;md5=cb7903f1e5c39ae838209e130dca270a \
                    file://COPYING.ASL20;md5=052f8a09206615ab07326ff8ce2d9d32\
"

SRC_URI = "http://www.rsyslog.com/files/download/rsyslog/${BPN}-${PV}.tar.gz \
           file://initscript \
           file://rsyslog.conf \
           file://rsyslog.logrotate \
"

SRC_URI[md5sum] = "750d552bdcbf255c85f464ffbe21168a"
SRC_URI[sha256sum] = "94346237ecfa22c9f78cebc3f18d59056f5d9846eb906c75beaa7e486f02c695"

inherit autotools pkgconfig systemd update-rc.d update-alternatives

EXTRA_OECONF += "--disable-uuid --disable-libgcrypt --enable-imfile"

do_install_append() {
    install -d "${D}${sysconfdir}/init.d"
    install -m 755 ${WORKDIR}/initscript ${D}${sysconfdir}/init.d/syslog.${BPN}
    install -m 644 ${WORKDIR}/rsyslog.conf ${D}${sysconfdir}/rsyslog.conf
    install -m 644 ${WORKDIR}/rsyslog.logrotate ${D}${sysconfdir}/logrotate.rsyslog
}

FILES_${PN} += "${bindir}"

INITSCRIPT_NAME = "syslog"
INITSCRIPT_PARAMS = "defaults"

# higher than sysklogd's 100
ALTERNATIVE_PRIORITY = "110"

ALTERNATIVE_${PN} = "syslogd syslog-conf syslog-logrotate"

ALTERNATIVE_LINK_NAME[syslogd] = "${base_sbindir}/syslogd"
ALTERNATIVE_TARGET[syslogd] = "${sbindir}/rsyslogd"
ALTERNATIVE_LINK_NAME[syslog-conf] = "${sysconfdir}/syslog.conf"
ALTERNATIVE_TARGET[syslog-conf] = "${sysconfdir}/rsyslog.conf"
ALTERNATIVE_LINK_NAME[syslog-logrotate] = "${sysconfdir}/logrotate.d/syslog"
ALTERNATIVE_TARGET[syslog-logrotate] = "${sysconfdir}/logrotate.rsyslog"

CONFFILES_${PN} = "${sysconfdir}/rsyslog.conf"

RPROVIDES_${PN} += "${PN}-systemd"
RREPLACES_${PN} += "${PN}-systemd"
RCONFLICTS_${PN} += "${PN}-systemd"
SYSTEMD_SERVICE_${PN} = "${BPN}.service"

RDEPENDS_${PN} += "logrotate"

# no syslog-init for systemd
python () {
    if bb.utils.contains('DISTRO_FEATURES', 'sysvinit', True, False, d):
        pn = d.getVar('PN', True)
        sysconfdir = d.getVar('sysconfdir', True)
        d.appendVar('ALTERNATIVE_%s' % (pn), ' syslog-init')
        d.setVarFlag('ALTERNATIVE_LINK_NAME', 'syslog-init', '%s/init.d/syslog' % (sysconfdir))
        d.setVarFlag('ALTERNATIVE_TARGET', 'syslog-init', '%s/init.d/syslog.%s' % (d.getVar('sysconfdir', True), d.getVar('BPN', True)))

    if bb.utils.contains('DISTRO_FEATURES', 'systemd', True, False, d):
        pn = d.getVar('PN', True)
        d.appendVar('ALTERNATIVE_%s' % (pn), ' syslog-service')
        d.setVarFlag('ALTERNATIVE_LINK_NAME', 'syslog-service', '%s/systemd/system/syslog.service' % (d.getVar('sysconfdir', True)))
        d.setVarFlag('ALTERNATIVE_TARGET', 'syslog-service', '%s/system/rsyslog.service' % (d.getVar('systemd_unitdir', True)))
}
