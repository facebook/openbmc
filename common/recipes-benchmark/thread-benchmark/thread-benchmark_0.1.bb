# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Exec benchmark"
DESCRIPTION = "Util for testing exec benchmark"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/GPL-2.0;md5=801f80980d171dd6425610833a22dbe6"

RDEPENDS_${PN} += "python3-core openbmc-utils"

SRC_URI = "file://Makefile \
           file://thread-benchmark.c \
           file://thread-benchmark-run.py \
          "
S = "${WORKDIR}"

binfiles = "thread-benchmark"

pkgdir = "thread-benchmark"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 thread-benchmark ${dst}/thread-benchmark
  install -m 755 thread-benchmark-run.py ${dst}/thread-benchmark-run.py
  ln -snf ../fbpackages/${pkgdir}/thread-benchmark ${bin}/thread-benchmark
  ln -snf ../fbpackages/${pkgdir}/thread-benchmark-run.py ${bin}/thread-benchmark-run.py
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/thread-benchmark ${prefix}/local/bin"
