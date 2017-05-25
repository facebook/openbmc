# Copyright 2015-present Facebook. All Rights Reserved.

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI += " file://setup-ipmbd.sh \
             file://run-ipmbd_3.sh \
             file://run-ipmbd_9.sh \
             file://run-ipmbd_11.sh \
           "
DEPENDS_append = " fbutils "

CFLAGS_prepend = " -DCONFIG_FBTTN"

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
  install -d ${D}${sysconfdir}/sv/ipmbd_3
  install -d ${D}${sysconfdir}/sv/ipmbd_9
  install -d ${D}${sysconfdir}/sv/ipmbd_11
  install -d ${D}${sysconfdir}/ipmbd_3
  install -d ${D}${sysconfdir}/ipmbd_9
  install -d ${D}${sysconfdir}/ipmbd_11
  install -m 755 setup-ipmbd.sh ${D}${sysconfdir}/init.d/setup-ipmbd.sh
  install -m 755 run-ipmbd_3.sh ${D}${sysconfdir}/sv/ipmbd_3/run
  install -m 755 run-ipmbd_9.sh ${D}${sysconfdir}/sv/ipmbd_9/run
  install -m 755 run-ipmbd_11.sh ${D}${sysconfdir}/sv/ipmbd_11/run
  update-rc.d -r ${D} setup-ipmbd.sh start 65 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/ipmbd ${prefix}/local/bin ${sysconfdir} "

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
