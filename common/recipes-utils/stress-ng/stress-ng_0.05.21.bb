DESCRIPTION = "stress-ng will stress test a computer system in various selectable ways. It \
was designed to exercise various physical subsystems of a computer as well as the various \
operating system kernel interfaces."
HOMEPAGE = "http://kernel.ubuntu.com/~cking/stress-ng/"

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=b234ee4d69f5fce4486a80fdaf4a4263"

SRC_URI = "http://kernel.ubuntu.com/~cking/tarballs/stress-ng/stress-ng-${PV}.tar.gz \
           file://Makefile.patch \
           "

SRC_URI[md5sum] = "aa22bcc892d9318c986be8525e2fbb8c"
SRC_URI[sha256sum] = "d8ac85cff80f12829130d0e48ff9833d7469c6736bb73295ab6d25ef9ae9a793"

S = "${WORKDIR}/stress-ng-${PV}"

do_install_append() {
    install -d ${D}${bindir}
    install -m 755 ${S}/stress-ng ${D}${bindir}/stress-ng
}
