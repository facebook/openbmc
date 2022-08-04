SRCBRANCH = "dev-5.0"
SRCREV = "f0f75fc33db7ac5b80fa0e6009f12ff7bd5995eb"

SRC_URI = "git://github.com/facebook/openbmc-linux.git;branch=${SRCBRANCH};protocol=https \
          "

LINUX_VERSION ?= "5.0.3"
LINUX_VERSION_EXTENSION ?= "-aspeed"

PR = "r1"
PV = "${LINUX_VERSION}"

include linux-aspeed.inc
require recipes-kernel/linux/linux-yocto.inc

LIC_FILES_CHKSUM = "file://COPYING;md5=bbea815ee2795b2f4230826c0c6b8814"

do_kernel_configme[depends] += "virtual/${TARGET_PREFIX}gcc:do_populate_sysroot"
KCONFIG_MODE="--alldefconfig"

S = "${WORKDIR}/git"
