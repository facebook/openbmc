DESCRIPTION = "Deliberately simple workload generator for POSIX systems. It \
imposes a configurable amount of CPU, memory, I/O, and disk stress on the system."
HOMEPAGE = "http://people.seas.harvard.edu/~apw/stress/"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=b234ee4d69f5fce4486a80fdaf4a4263"

SRC_URI = "http://pkgs.fedoraproject.org/repo/pkgs/stress/stress-${PV}.tar.gz/a607afa695a511765b40993a64c6e2f4/stress-${PV}.tar.gz \
           file://texinfo.patch \
           "

SRC_URI[md5sum] = "890a4236dd1656792f3ef9a190cf99ef"
SRC_URI[sha256sum] = "057e4fc2a7706411e1014bf172e4f94b63a12f18412378fca8684ca92408825b"

inherit autotools
