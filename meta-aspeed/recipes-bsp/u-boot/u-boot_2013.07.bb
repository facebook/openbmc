require u-boot.inc

LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://COPYING;md5=1707d6db1d42237583f50183a5651ecb \
                    file://README;beginline=1;endline=22;md5=78b195c11cb6ef63e6985140db7d7bab"

# This revision corresponds to the tag "v2013.07"
# We use the revision in order to avoid having to fetch it from the repo during parse
SRCREV = "9f881c00b376e1b56c6a661f34b32974738c04e0"

PV = "v2013.07-openbmc"

SRC_URI = "git://github.com/theopolis/u-boot-openbmc.git;branch=${PV};protocol=git \
           file://fw_env.config \
          "

S = "${WORKDIR}/git"

PACKAGE_ARCH = "${MACHINE_ARCH}"
