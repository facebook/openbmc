FILESEXTRAPATHS:append := "${THISDIR}/${PN}:"

SRC_URI += " \
            file://logrotate-3.9.1/examples/logrotate-default \
            file://logrotate-3.9.1/examples/wtmp_default \
           "

# We need to rotate every hour or could quickly run out of RAM.
LOGROTATE_SYSTEMD_TIMER_BASIS = "hourly"
LOGROTATE_SYSTEMD_TIMER_ACCURACY = "30m"

do_install:append() {
    install -p -m 644 ${WORKDIR}/logrotate-3.9.1/examples/logrotate-default ${D}${sysconfdir}/logrotate.conf
    install -p -m 644 ${WORKDIR}/logrotate-3.9.1/examples/wtmp_default ${D}${sysconfdir}/logrotate.d/wtmp

    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        # XXX For now we build Yocto with systemd enabled and sysvinit
        # compat to allow remaining sysvinit scripts to continue working.
        # With both systemd & sysvinit compat on, the logrotate recipe gets
        # it wrong and installs both the crontab entry and systemd timer.
        # When sysvinit compat is removed then this can go away.
        rm -f ${D}${sysconfdir}/cron.daily/logrotate
    else
        # logrotate by default installed as cron.daily job by logrotate recipe
        # but current all platforms setup  do log rotate hourly "reusing" cron.daily
        # to avoid confusing, move it to cron.hourly
        bbnote "move logrotate from cron.daily to cron.hourly"
        install -d ${D}${sysconfdir}/cron.hourly
        mv ${D}${sysconfdir}/cron.daily/logrotate ${D}${sysconfdir}/cron.hourly/logrotate
    fi
}
