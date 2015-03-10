
DESCRIPTION = "Linux Kernel for Aspeed"
SECTION = "kernel"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=d7810fab7487fb0aad327b76f1be7cd7"

COMPATIBLE_MACHINE = "aspeed"

do_compile[depends] = "libgcc:do_populate_sysroot"

inherit kernel

# auto load the following modules
module_autoload_tun = "tun"
module_autoload_at24 = "at24"
module_autoload_ads7828 = "ads7828"
module_autoload_pcf8574 = "pcf8574"
module_autoload_max127 = "max127"
module_conf_max127 = "options max127 scaling=24414"
module_autoload_pmbus_core = "pmbus_core"
module_autoload_pfe1100 = "pfe1100"
module_autoload_fb_panther_plus = "fb_panther_plus"
module_autoload_adm1275 = "adm1275"

# Do not install kernel in rootfs
do_install[postfuncs] += "remove_kernel_image_from_rootfs"
remove_kernel_image_from_rootfs() {
    rm -rf ${D}/boot/uImage*
}
pkg_postinst_kernel-image() {
}
