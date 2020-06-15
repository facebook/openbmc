FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += " \
    file://plat/meson.build \
    "

DEPENDS += " \
    libbic \
    libsensor-correction \
    libobmc-i2c \
    libmisc-utils \
    liblog \
    "

# These shouldn't be needed but are because we aren't properly versioning the
# shared libraries contained in these recipes.
RDEPENDS_${PN} += " \
    libbic \
    liblog \
    libmisc-utils \
    libobmc-i2c \
    libsensor-correction \
    "
