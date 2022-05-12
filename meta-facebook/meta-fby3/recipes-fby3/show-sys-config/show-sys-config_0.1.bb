# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Show System Configuration Utility"
DESCRIPTION = "Util for showing system configuration"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://show_sys_config.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

LOCAL_URI = " \
    file://show_sys_config.c \
    file://Makefile \
    "

binfiles = "show_sys_config \
           "

pkgdir = "show_sys_config"

CFLAGS += " -Wall -Werror "
LDFLAGS = "-lipmi -lipmb -lbic -lpal -lfby3_common -ljansson"
DEPENDS = "libbic libpal libipmi libipmb libfby3-common jansson"
RDEPENDS:${PN} += "libbic libpal libipmi libipmb libfby3-common jansson"

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

FILES:${PN} = "${FBPACKAGEDIR}/show_sys_config ${prefix}/local/bin"
