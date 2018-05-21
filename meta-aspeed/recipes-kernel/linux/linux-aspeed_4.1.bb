
SRCBRANCH = "openbmc/helium/4.1"
SRCREV = "AUTOINC"

SRC_URI = "git://github.com/theopolis/linux.git;branch=${SRCBRANCH};protocol=https \
          file://patch-4.1.15/0000-Create-Mavericks-OpenBMC.patch \
          "

LINUX_VERSION ?= "4.1.51"
LINUX_VERSION_EXTENSION ?= "-aspeed"

PR = "r1"
PV = "${LINUX_VERSION}"

S = "${WORKDIR}/git"

include linux-aspeed.inc
