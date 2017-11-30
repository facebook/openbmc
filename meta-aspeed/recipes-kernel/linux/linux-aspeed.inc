
DESCRIPTION = "Linux Kernel for Aspeed"
SECTION = "kernel"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=d7810fab7487fb0aad327b76f1be7cd7"

COMPATIBLE_MACHINE = "aspeed"

do_compile[depends] = "libgcc:do_populate_sysroot"

inherit kernel
require recipes-kernel/linux/linux-yocto.inc

# Do not install kernel in rootfs
do_install[postfuncs] += "remove_kernel_image_from_rootfs"
remove_kernel_image_from_rootfs() {
    rm -rf ${D}/boot/uImage*
}

pkg_postinst_kernel-image() {
}
