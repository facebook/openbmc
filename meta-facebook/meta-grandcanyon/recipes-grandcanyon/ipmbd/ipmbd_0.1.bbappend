# Copyright 2020-present Facebook. All Rights Reserved.

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI += " file://setup-ipmbd.sh \
             file://run-ipmbd_2.sh \
             file://run-ipmbd_7.sh \
             file://run-ipmbd_10.sh \
           "
#DEPENDS_append = " plat-utils "
CFLAGS_prepend = " -DTIMEOUT_IPMB=3 "

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
  install -d ${D}${sysconfdir}/sv/ipmbd_2
  install -d ${D}${sysconfdir}/sv/ipmbd_7
  install -d ${D}${sysconfdir}/sv/ipmbd_10
  install -m 755 setup-ipmbd.sh ${D}${sysconfdir}/init.d/setup-ipmbd.sh
  install -m 755 run-ipmbd_2.sh ${D}${sysconfdir}/sv/ipmbd_2/run
  install -m 755 run-ipmbd_7.sh ${D}${sysconfdir}/sv/ipmbd_7/run
  install -m 755 run-ipmbd_10.sh ${D}${sysconfdir}/sv/ipmbd_10/run
  update-rc.d -r ${D} setup-ipmbd.sh start 65 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/ipmbd ${prefix}/local/bin ${sysconfdir} "

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
