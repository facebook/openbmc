SRCBRANCH = "dev-5.3"
SRCREV = "AUTOINC"

SRC_URI = "git://github.com/facebook/openbmc-linux.git;branch=${SRCBRANCH};protocol=https \
          "

LINUX_VERSION ?= "5.3.8"
LINUX_VERSION_EXTENSION ?= "-aspeed"
LIC_FILES_CHKSUM = "file://COPYING;md5=bbea815ee2795b2f4230826c0c6b8814"

PR = "r1"
PV = "${LINUX_VERSION}"

include linux-aspeed.inc

S = "${WORKDIR}/git"

#
# Note: below line is needed to bitbake linux kernel 5.2 and higher kernel
# versions. The fix is already included in the latest yocto releases (refer
# to meta/classes/kernel.bbclass), thus it's safe to remove the line when we
# move out of rocko.
#
FILES_kernel-base += "${nonarch_base_libdir}/modules/${KERNEL_VERSION}/modules.builtin.modinfo"
