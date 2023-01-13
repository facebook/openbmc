FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LOCAL_URI += " \
    file://setup-mctpd.sh \
    file://run-mctpd_8.sh \
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
  install -d ${D}${sysconfdir}/sv/mctpd_8
  install -m 755 setup-mctpd.sh ${D}${sysconfdir}/init.d/setup-mctpd.sh
  install -m 755 run-mctpd_8.sh ${D}${sysconfdir}/sv/mctpd_8/run
  update-rc.d -r ${D} setup-mctpd.sh start 68 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/mctpd ${prefix}/local/bin ${sysconfdir} "
