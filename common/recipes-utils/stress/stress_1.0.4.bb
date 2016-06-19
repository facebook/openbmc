DESCRIPTION = "Deliberately simple workload generator for POSIX systems. It \
imposes a configurable amount of CPU, memory, I/O, and disk stress on the system."
HOMEPAGE = "http://people.seas.harvard.edu/~apw/stress/"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=d32239bcb673463ab874e80d47fae504"

SRC_URI = "http://pkgs.fedoraproject.org/repo/pkgs/stress/stress-${PV}.tar.gz/a607afa695a511765b40993a64c6e2f4/stress-${PV}.tar.gz \
           file://texinfo.patch \
           "

SRC_URI[md5sum] = "a607afa695a511765b40993a64c6e2f4"
SRC_URI[sha256sum] = "369c997f65e8426ae8b318d4fdc8e6f07a311cfa77cc4b25dace465c582163c0"

inherit autotools
