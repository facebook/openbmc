FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LOCAL_URI += " \
    file://setup-mctpd.sh \
    file://run-mctpd_3.sh \
    file://run-mctpd_4.sh \
    "
DEPENDS:append = " plat-utils update-rc.d-native"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 mctpd ${dst}/mctpd
  ln -snf ../fbpackages/${pkgdir}/mctpd ${bin}/mctpd
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/mctpd_3
  install -d ${D}${sysconfdir}/sv/mctpd_4
  install -m 755 setup-mctpd.sh ${D}${sysconfdir}/init.d/setup-mctpd.sh
  install -m 755 run-mctpd_3.sh ${D}${sysconfdir}/sv/mctpd_3/run
  install -m 755 run-mctpd_4.sh ${D}${sysconfdir}/sv/mctpd_4/run
  update-rc.d -r ${D} setup-mctpd.sh start 57 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/mctpd ${prefix}/local/bin ${sysconfdir} "
