FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

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

RDEPENDS_${PN} += " \
    libbic \
    libexp \
    libmctp \
    libfbttn-common \
    libfbttn-fruid \
    libfbttn-sensor \
    libnvme-mi \
    "

CFLAGS += " -DCONFIG_FBTTN=1"
