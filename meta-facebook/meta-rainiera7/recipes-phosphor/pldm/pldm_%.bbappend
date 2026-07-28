EXTRA_OEMESON:append = " -Dfw-update-pkg-inotify=enabled "
EXTRA_OEMESON:append = " -Dmctp-recovery=disabled "
EXTRA_OEMESON:append = " -Denable-fallback-terminus-name=disabled "
EXTRA_OEMESON:append = " -Ddiscovery-fru-data-from-terminus=disabled "
EXTRA_OEMESON:append = " -Ddbus-timeout-value=30 "

do_install:append:openbmc-fb-lf() {
    rm -f ${D}/usr/share/pldm/host_eid
}
