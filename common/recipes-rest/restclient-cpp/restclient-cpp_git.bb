LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=875d6e9ec1cb59099f4d9da1e81d1e91"

SRC_URI = " \
    git://github.com/mrtazz/restclient-cpp.git;branch=master;protocol=https \
    file://0001-Fix-missing-cstdint-include.patch \
    "
SRCREV = "b782bd26539a3d1a8edcb6d8a3493b111f8fac66"

S = "${WORKDIR}/git"

PV = "0.5.2"
PR = "r1"

DEPENDS = "curl"

inherit pkgconfig cmake
