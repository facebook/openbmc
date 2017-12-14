FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
            file://busybox.cfg \
            file://setup_crond.sh \
           "

DEPENDS_append = " update-rc.d-native"

do_install_append() {
  install -d ${D}${sysconfdir}/init.d
  install -m 755 ../setup_crond.sh ${D}${sysconfdir}/init.d/setup_crond.sh
  update-rc.d -r ${D} setup_crond.sh start 97 5 .
}
