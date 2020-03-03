# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Show System Configuration Utility"
DESCRIPTION = "Util for showing system configuration"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://show_sys_config.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://show_sys_config.c \
           file://Makefile \
          "

S = "${WORKDIR}"

binfiles = "show_sys_config \
           "

pkgdir = "show_sys_config"

CFLAGS += " -Wall -Werror "
LDFLAGS = "-lipmi -lipmb -lbic -lpal -lfby3_common -ljansson"
DEPENDS = "libbic libpal libipmi libipmb libfby3-common jansson"
RDEPENDS_${PN} += "libbic libpal libipmi libipmb libfby3-common jansson"

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

FILES_${PN} = "${FBPACKAGEDIR}/show_sys_config ${prefix}/local/bin"

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
