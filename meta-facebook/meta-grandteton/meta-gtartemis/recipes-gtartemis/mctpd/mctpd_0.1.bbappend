FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LOCAL_URI += " \
    file://run-mctpd_9.sh \
    "

do_install:append() {
  install -d ${D}${sysconfdir}/sv/mctpd_9
  install -m 755 run-mctpd_9.sh ${D}${sysconfdir}/sv/mctpd_9/run
}
