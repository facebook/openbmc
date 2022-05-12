# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Exec benchmark"
DESCRIPTION = "Util for testing exec benchmark"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-only"

# The license GPL-2.0 was removed in Hardknott.
# Use GPL-2.0-only instead.
def lic_file_name(d):
    distro = d.getVar('DISTRO_CODENAME', True)
    if distro in [ 'rocko', 'dunfell' ]:
        return "GPL-2.0;md5=801f80980d171dd6425610833a22dbe6"

    return "GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

LIC_FILES_CHKSUM = "\
    file://${COREBASE}/meta/files/common-licenses/${@lic_file_name(d)} \
    "

RDEPENDS:${PN} += "python3-core openbmc-utils"

LOCAL_URI = " \
    file://Makefile \
    file://thread-benchmark.c \
    file://thread-benchmark-run.py \
    "

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

FILES:${PN} = "${FBPACKAGEDIR}/thread-benchmark ${prefix}/local/bin"
