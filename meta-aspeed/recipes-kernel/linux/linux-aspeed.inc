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

do_configure_prepend() {
    # in kernel.bbclass::kernel_do_configure(), the code only copies defconfig to
    # .config if "${B}/.config" is not there. That causes issue that if defconfig
    # is changed, the kernel build will not pick up the new kernel config.
    # We copy defconfig all the time to avoid this bug.
    if [ -f "${WORKDIR}/defconfig" ]; then
        cp -p "${WORKDIR}/defconfig" "${B}/.config"
    fi
}
