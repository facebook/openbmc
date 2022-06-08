SRC_URI = "git://github.com/mrtazz/restclient-cpp.git;branch=master;protocol=https \
          "
SRCREV = "b782bd26539a3d1a8edcb6d8a3493b111f8fac66"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=875d6e9ec1cb59099f4d9da1e81d1e91"

PR = "r3"

S = "${WORKDIR}/git"

DEPENDS = "curl"
RDEPENDS_${PN} = "libcurl"

inherit pkgconfig cmake

# Specify any options you want to pass to cmake using EXTRA_OECMAKE:
EXTRA_OECMAKE = ""
