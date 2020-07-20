FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://plat/meson.build \
    "

DEPENDS += " \
    libgpio \
    libsensor-correction \
    "

# These shouldn't be needed but are because we aren't properly versioning the
# shared libraries contained in these recipes.
RDEPENDS_${PN} += " \
    libgpio \
    libsensor-correction \
    "
