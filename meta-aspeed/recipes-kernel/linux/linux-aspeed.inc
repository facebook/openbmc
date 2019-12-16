DESCRIPTION = "Linux Kernel for Aspeed"
SECTION = "kernel"
LICENSE = "GPLv2"

COMPATIBLE_MACHINE = "aspeed"

do_compile[depends] = "libgcc:do_populate_sysroot"

inherit kernel

# Do not install kernel in rootfs
do_install[postfuncs] += "remove_kernel_image_from_rootfs"
remove_kernel_image_from_rootfs() {
    rm -rf ${D}/boot/${KERNEL_IMAGETYPE}*
}

pkg_postinst_kernel-image() {
}
