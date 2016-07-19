DESCRIPTION = "Google gflags"
HOMEPAGE = "http://gflags.github.io/gflags"

LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://COPYING.txt;md5=c80d1a3b623f72bb85a4c75b556551df"

# v2.1.2
SRCREV = "1a02f2851ee3d48d32d2c8f4d8f390a0bc25565c"
SRC_URI = "git://github.com/gflags/gflags.git;protocol=https;branch=release"

SRC_URI[md5sum] = "ac432de923f9de1e9780b5254884599f"
SRC_URI[sha256sum] = "d8331bd0f7367c8afd5fcb5f5e85e96868a00fd24b7276fa5fcee1e5575c2662"

inherit cmake

S = "${WORKDIR}/git"

EXTRA_OECMAKE = "-DBUILD_SHARED_LIBS=ON"

RDEPENDS_${PN} = "bash"

FILES_${PN} = "/usr"

BBCLASSEXTEND = "native"