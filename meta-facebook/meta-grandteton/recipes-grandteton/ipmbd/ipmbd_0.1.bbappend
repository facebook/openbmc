# Copyright 2015-present Facebook. All Rights Reserved.

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LOCAL_URI += " \
    file://setup-ipmbd.sh \
    file://run-ipmbd_6.sh \
    file://run-ipmbd_14.sh \
    "
DEPENDS:append = " plat-utils update-rc.d-native"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 ipmbd ${dst}/ipmbd
  ln -snf ../fbpackages/${pkgdir}/ipmbd ${bin}/ipmbd
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/ipmbd_6
  install -d ${D}${sysconfdir}/sv/ipmbd_14
  install -m 755 setup-ipmbd.sh ${D}${sysconfdir}/init.d/setup-ipmbd.sh
  install -m 755 run-ipmbd_6.sh ${D}${sysconfdir}/sv/ipmbd_6/run
  install -m 755 run-ipmbd_14.sh ${D}${sysconfdir}/sv/ipmbd_14/run
  update-rc.d -r ${D} setup-ipmbd.sh start 65 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/ipmbd ${prefix}/local/bin ${sysconfdir} "
