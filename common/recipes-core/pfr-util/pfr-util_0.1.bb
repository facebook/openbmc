# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "PFR Utility"
DESCRIPTION = "Utility for PFR"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://pfr-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

LOCAL_URI = " \
    file://Makefile \
    file://pfr-util.c \
    "

binfiles = "pfr-util"

pkgdir = "pfr-util"

DEPENDS = "openssl libpal libobmc-i2c libfpga"
RDEPENDS:${PN} = "openssl libpal libobmc-i2c libfpga"
LDFLAGS += "-lcrypto -lpal -lobmc-i2c -lfpga"

CFLAGS:append:mf-fb-bic = " -DCONFIG_BIC"
LDFLAGS:append:mf-fb-bic = " -lbic"
DEPENDS:append:mf-fb-bic = " libbic"
RDEPENDS:${PN}:append:mf-fb-bic = " libbic"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  for f in ${binfiles}; do
    install -m 755 $f ${dst}/$f
    ln -snf ../fbpackages/${pkgdir}/$f ${bin}/$f
  done
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/pfr-util ${prefix}/local/bin"
