SRCBRANCH = "dev-5.6"
SRCREV = "AUTOINC"

SRC_URI = "git://github.com/facebook/openbmc-linux.git;branch=${SRCBRANCH};protocol=https \
          "

LINUX_VERSION ?= "5.6.19"
LINUX_VERSION_EXTENSION ?= "-aspeed"

PR = "r1"
PV = "${LINUX_VERSION}"

include linux-aspeed.inc
require recipes-kernel/linux/linux-yocto.inc

LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"

do_kernel_configme[depends] += "virtual/${TARGET_PREFIX}gcc:do_populate_sysroot"
KCONFIG_MODE="--alldefconfig"

S = "${WORKDIR}/git"

#
# Note: below fixup is needed to bitbake linux kernel 5.2 or higher kernel
# versions using yocto-rocko. It's usually needed by ast2400 BMC platforms
# because they have to stay with u-boot-v2016.07 which cannot be compiled
# by newer (than rocko) yocto versions.
#
python () {
    if d.getVar('DISTRO_CODENAME') == 'rocko':
        d.appendVar(
            'FILES_kernel-base',
            ' ${nonarch_base_libdir}/modules/${KERNEL_VERSION}/modules.builtin.modinfo')
}
