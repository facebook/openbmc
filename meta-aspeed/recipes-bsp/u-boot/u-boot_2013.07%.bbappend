FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://fw_env.config \
            file://patch-2013.07/0000-u-boot-aspeed-064.patch \
            file://patch-2013.07/0001-u-boot-openbmc.patch \
            file://patch-2013.07/0002-Create-snapshot-of-OpenBMC.patch \
           "

# Do not install u-boot in rootfs
do_install[postfuncs] += "remove_uboot_from_rootfs"
remove_uboot_from_rootfs() {
    rm -rf ${D}/boot/u-boot*
}
