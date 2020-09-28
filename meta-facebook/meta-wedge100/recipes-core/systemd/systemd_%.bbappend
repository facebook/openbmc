do_install_append() {
    # The journal will store its logs in tmpfs and yocto's default
    # will kill the BMC
    sed -i -e 's/RuntimeMaxUse.*/RuntimeMaxUse=20M/' ${D}${sysconfdir}/systemd/journald.conf
    sed -i -e 's/.*ForwardToSyslog.*/ForwardToSyslog=yes/' ${D}${sysconfdir}/systemd/journald.conf
}
