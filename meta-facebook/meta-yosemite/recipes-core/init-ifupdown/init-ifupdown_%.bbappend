
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://setup-dhc6.sh \
            file://run-dhc6.sh \
           "

do_install_append() {
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/dhc6
  install -m 755 setup-dhc6.sh ${D}${sysconfdir}/init.d/setup-dhc6.sh
  install -m 755 run-dhc6.sh ${D}${sysconfdir}/sv/dhc6/run
  update-rc.d -r ${D} setup-dhc6.sh start 02 5 .
}
