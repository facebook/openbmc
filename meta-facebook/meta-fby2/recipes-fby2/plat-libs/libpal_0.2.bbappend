FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += " \
    file://plat/meson.build \
    "

DEPENDS += " \
    jansson \
    libbic \
    libfby2-common \
    libfby2-fruid \
    libfby2-sensor \
    libfruid \
    libncsi \
    libobmc-i2c \
    "

# These shouldn't be needed but are because we aren't properly versioning the
# shared libraries contained in these recipes.
RDEPENDS_${PN} += " \
    libbic \
    libfby2-common \
    libfby2-fruid \
    libfby2-sensor \
    libfruid \
    libncsi \
    libobmc-i2c \
    "
