FILESEXTRAPATHS:prepend := "${THISDIR}/files/pal:"

SRC_URI += "file://plat/meson.build"

DEPENDS += " \
    libbic \
    libexp \
    libfbttn-common \
    libfbttn-fruid \
    libfbttn-sensor \
    libmctp \
    libnvme-mi \
    "

RDEPENDS:${PN} += " \
    libbic \
    libexp \
    libmctp \
    libfbttn-common \
    libfbttn-fruid \
    libfbttn-sensor \
    libnvme-mi \
    "
