# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Health Monitoring Daemon"
DESCRIPTION = "Daemon for BMC Health monitoring"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://healthd.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://Makefile \
           file://healthd.c \
           file://pfr_monitor.c \
           file://setup-healthd.sh \
           file://run-healthd.sh \
           file://healthd-config.json \
          "
S = "${WORKDIR}"

LDFLAGS =+ " -lpal -ljansson -lkv -lwatchdog -lvbs -lobmc-i2c "

DEPENDS =+ " libpal jansson libobmc-i2c libkv update-rc.d-native libwatchdog libvbs"

binfiles = "healthd"

pkgdir = "healthd"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 healthd ${dst}/healthd
  ln -snf ../fbpackages/${pkgdir}/healthd ${bin}/healthd

  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/healthd
  install -d ${D}${sysconfdir}/healthd
  install -m 644 healthd-config.json ${D}${sysconfdir}/healthd-config.json
  install -m 755 setup-healthd.sh ${D}${sysconfdir}/init.d/setup-healthd.sh
  install -m 755 run-healthd.sh ${D}${sysconfdir}/sv/healthd/run
  update-rc.d -r ${D} setup-healthd.sh start 91 5 .
}

RDEPENDS_${PN} =+ " libpal jansson libwatchdog libvbs libobmc-i2c "

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/healthd ${prefix}/local/bin ${sysconfdir} "
