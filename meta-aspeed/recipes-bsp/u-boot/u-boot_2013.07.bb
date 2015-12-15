require u-boot.inc

LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://COPYING;md5=1707d6db1d42237583f50183a5651ecb \
                    file://README;beginline=1;endline=22;md5=78b195c11cb6ef63e6985140db7d7bab"

# This revision corresponds to the tag "v2013.07"
# We use the revision in order to avoid having to fetch it from the repo during parse
SRCREV = "62c175fbb8a0f9a926c88294ea9f7e88eb898f6c"

PV = "v2013.07"

SRC_URI = "git://git.denx.de/u-boot.git;branch=master \
           file://fw_env.config \
           file://patch-2013.07/0000-u-boot-aspeed-064.patch \
           file://patch-2013.07/0001-u-boot-openbmc.patch \
           file://patch-2013.07/0002-Create-snapshot-of-OpenBMC.patch;striplevel=6 \
           file://patch-2013.07/0003-u-boot-snapshot-of-OpenBMC-f926614.patch;striplevel=6 \
          "

S = "${WORKDIR}/git"

PACKAGE_ARCH = "${MACHINE_ARCH}"
