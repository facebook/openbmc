FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

EXTRA_OEMESON:append = " -Dmaximum-transfer-size=150 "
EXTRA_OEMESON:append = " -Dsensor-polling-time=2000 "
EXTRA_OEMESON:append = " -Ddiscovery-fru-data-from-terminus=disabled "
EXTRA_OEMESON:append = " -Dmctp-recovery=disabled "
EXTRA_OEMESON:append = " -Dfw-update-pkg-inotify=enabled "
EXTRA_OEMESON:append = " -Denable-fallback-terminus-name=disabled "

do_install:append:openbmc-fb-lf() {
    rm -f ${D}/usr/share/pldm/host_eid
}
