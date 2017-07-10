DESCRIPTION = "The gflags package contains a C++ library that implements commandline flags processing. It includes built-in support for standard types such as string and the ability to define flags in the source file in which they are used"

HOMEPAGE = "https://github.com/gflags/gflags"

LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://COPYING.txt;md5=c80d1a3b623f72bb85a4c75b556551df"

SRC_URI = "https://github.com/gflags/gflags/archive/v${PV}.tar.gz;downloadfilename=${PN}-${PV}.tar.gz"
SRC_URI[md5sum] = "b99048d9ab82d8c56e876fb1456c285e"
SRC_URI[sha256sum] = "466c36c6508a451734e4f4d76825cf9cd9b8716d2b70ef36479ae40f08271f88"

FILES_${PN}-dev += "${libdir}/cmake"

inherit cmake

EXTRA_OECMAKE="-DBUILD_SHARED_LIBS=ON -DREGISTER_INSTALL_PREFIX=OFF -DLIB_INSTALL_DIR=${baselib}"

PACKAGES =+ "${PN}-bash-completion"
FILES_${PN}-bash-completion += "${bindir}/gflags_completions.sh"
RDEPENDS_${PN}-bash-completion = "bash-completion"
