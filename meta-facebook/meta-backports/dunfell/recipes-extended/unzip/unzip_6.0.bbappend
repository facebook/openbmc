# unzip's unix/configure feature probes are link tests that use implicit
# function declarations / implicit int. GCC >= 14 (newer build hosts) makes
# those hard errors, so the probes fail to compile and configure spuriously
# emits -DNO_DIR/-DZMEM/-DNO_ERRNO, activating a broken "typedef FILE DIR"
# fallback in unix/unix.c that collides with <dirent.h> and breaks the native
# build. The patch restores warning behaviour so detection works again.
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://0001-configure-don-t-let-implicit-decl-errors-break-probes.patch \
"
