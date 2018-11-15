SRCBRANCH = "dev-4.17"
SRCREV = "AUTOINC"

SRC_URI = "git://github.com/facebook/openbmc-linux.git;branch=${SRCBRANCH};protocol=https \
          "

LINUX_VERSION ?= "4.17.2"
LINUX_VERSION_EXTENSION ?= "-aspeed"
LIC_FILES_CHKSUM = "file://COPYING;md5=bbea815ee2795b2f4230826c0c6b8814"

PR = "r1"
PV = "${LINUX_VERSION}"

include linux-aspeed.inc

S = "${WORKDIR}/git"
