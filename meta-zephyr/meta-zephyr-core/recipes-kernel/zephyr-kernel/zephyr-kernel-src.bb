include zephyr-kernel-src.inc

ZEPHYR_TEST_SRCDIR = "tests/legacy/kernel/"

IMAGE_NO_MANIFEST = "1"
INHIBIT_DEFAULT_DEPS = "1"

do_configure[noexec] = "1"
do_compile[noexec] = "1"
do_install () {
    kerneldir=${D}/usr/src/zephyr
    install -d $kerneldir
    cp -r ${S}/* $kerneldir
}

PACKAGES = "${PN}"
FILES:${PN} = "/usr/src/zephyr"

SYSROOT_DIRS += "/usr/src/zephyr"
