FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += " \
    file://plat/meson.build \
    "

DEPENDS += " \
    libobmc-sensors \
    "
RDEPENDS_${PN} += " \
    libobmc-sensors \
    "
