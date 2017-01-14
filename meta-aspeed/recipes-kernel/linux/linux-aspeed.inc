
DESCRIPTION = "Linux Kernel for Aspeed"
SECTION = "kernel"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=d7810fab7487fb0aad327b76f1be7cd7"

COMPATIBLE_MACHINE = "aspeed"

do_compile[depends] = "libgcc:do_populate_sysroot"

inherit kernel
require recipes-kernel/linux/linux-yocto.inc
require recipes-bsp/u-boot/verified-boot.inc

do_configure[prefuncs] += "configure_spi_layout"
configure_spi_layout() {
    # If the VERIFIED_BOOT flag is changed the state will need to be cleaned.
    if [ "x${VERIFIED_BOOT}" != "x" ] ; then
        defconfig_option_on CONFIG_AST_DUAL_FLASH ../defconfig
        if [ -f ".config" ]; then
            defconfig_option_on CONFIG_AST_DUAL_FLASH .config
        fi
    else
        defconfig_option_off CONFIG_AST_DUAL_FLASH ../defconfig
        if [ -f ".config" ]; then
            defconfig_option_off CONFIG_AST_DUAL_FLASH .config
        fi
    fi
}

# Do not install kernel in rootfs
do_install[postfuncs] += "remove_kernel_image_from_rootfs"
remove_kernel_image_from_rootfs() {
    rm -rf ${D}/boot/uImage*
}

pkg_postinst_kernel-image() {
}
