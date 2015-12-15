INHIBIT_DEFAULT_DEPS = "1"
LICENSE = "GPLv2 & LGPLv2.1"
HOMEPAGE = "http://www.eglibc.org/"

do_fetch[noexec] = "1"
do_unpack[noexec] = "1"
do_patch[noexec] = "1"
do_configure[noexec] = "1"
do_compile[noexec] = "1"

do_install[depends] += "virtual/${MLPREFIX}libc:do_populate_sysroot"

