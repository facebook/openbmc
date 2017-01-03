require u-boot.inc

LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://COPYING;md5=1707d6db1d42237583f50183a5651ecb \
                    file://README;beginline=1;endline=22;md5=78b195c11cb6ef63e6985140db7d7bab"

# This revision corresponds to the tag "openbmc/fido/v2013.07"
# We use the revision in order to avoid having to fetch it from the repo during parse
SRCBRANCH = "openbmc/fido/v2013.07"
SRCREV = "AUTOINC"

SRC_URI = "git://github.com/theopolis/u-boot.git;branch=${SRCBRANCH};protocol=https \
           file://fw_env.config \
          "

PV = "v2013.07"
S = "${WORKDIR}/git"

PACKAGE_ARCH = "${MACHINE_ARCH}"
