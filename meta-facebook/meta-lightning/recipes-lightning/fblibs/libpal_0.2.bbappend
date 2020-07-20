FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += " \
    file://plat/meson.build \
    "

DEPENDS += " \
    liblightning-common \
    liblightning-fruid \
    liblightning-sensor \
    liblightning-flash \
    liblightning-gpio \
    libnvme-mi \
    libobmc-i2c \
    "

# These shouldn't be needed but are because we aren't properly versioning the
# shared libraries contained in these recipes.
RDEPENDS_${PN} += " \
    liblightning-common \
    liblightning-fruid \
    liblightning-sensor \
    liblightning-flash \
    liblightning-gpio \
    libnvme-mi \
    libobmc-i2c \
    "
