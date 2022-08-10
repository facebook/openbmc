SRCBRANCH = "dev-${LINUX_VERSION_MAJOR}.${LINUX_VERSION_MINOR}"
SRCREV = "AUTOINC"
SRC_URI = "git://github.com/facebook/openbmc-linux.git;branch=${SRCBRANCH};protocol=https \
          "
PV = "${LINUX_VERSION}+git${SRCPV}"
S = "${WORKDIR}/git"
