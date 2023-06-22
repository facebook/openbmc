require pseudo.inc

SRC_URI = "git://git.yoctoproject.org/pseudo \
           file://0001-configure-Prune-PIE-flags.patch \
           file://fallback-passwd \
           file://fallback-group \
           file://moreretries.patch \
           file://toomanyfiles.patch \
           file://0001-maketables-wrappers-use-Python-3.patch \
           file://0001-Add-statx.patch \
           file://0001-realpath.c-Remove-trailing-slashes.patch \
           file://0001-OE-core-PATCH-linux-portdefs.h-Fix-pseudo-to-work-wi.patch \
           file://0002-OE-core-PATCH-pseudo-Fix-build-with-gcc-10.patch \
           "

SRCREV = "060058bb29f70b244e685b3c704eb0641b736f73"
S = "${WORKDIR}/git"
PV = "1.9.0+git${SRCPV}"

# error: use of undeclared identifier '_STAT_VER'
COMPATIBLE_HOST_libc-musl = 'null'
