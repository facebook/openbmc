EXTRA_OEMESON:append = " -Dfw-update-pkg-inotify=enabled "
EXTRA_OEMESON:append = " -Dmctp-recovery=disabled "
EXTRA_OEMESON:append = " -Denable-fallback-terminus-name=disabled "
EXTRA_OEMESON:append = " -Ddiscovery-fru-data-from-terminus=disabled "
EXTRA_OEMESON:append = " -Ddbus-timeout-value=30 "
EXTRA_OEMESON:append = " -Dmaximum-transfer-size=128 "

PACKAGECONFIG:append = " oem-arm"

do_install:append:openbmc-fb-lf() {
    rm -f ${D}/usr/share/pldm/host_eid
}
