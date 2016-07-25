
DESCRIPTION = "Linux Kernel for Aspeed"
SECTION = "kernel"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=d7810fab7487fb0aad327b76f1be7cd7"

COMPATIBLE_MACHINE = "aspeed"

do_compile[depends] = "libgcc:do_populate_sysroot"

inherit kernel

# auto load the following modules
KERNEL_MODULE_AUTOLOAD += "                     \
 adm1275                                        \
 ads7828                                        \
 at24                                           \
 max127                                         \
 rtc-pcf8563                                    \
 pcf8574                                        \
 pfe1100                                        \
 pmbus_core                                     \
 tun                                            \
"

KERNEL_MODULE_PROBECONF += "                    \
 max127                                         \
"
module_conf_max127 = "options max127 scaling=24414"

# Do not install kernel in rootfs
do_install[postfuncs] += "remove_kernel_image_from_rootfs"
remove_kernel_image_from_rootfs() {
    rm -rf ${D}/boot/uImage*
}
pkg_postinst_kernel-image() {
}
