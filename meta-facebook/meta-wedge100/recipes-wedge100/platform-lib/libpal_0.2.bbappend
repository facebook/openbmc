FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://plat/meson.build \
    "

DEPENDS += " \
    libobmc-i2c \
    libsensor-correction \
    "

# These shouldn't be needed but are because we aren't properly versioning the
# shared libraries contained in these recipes.
RDEPENDS_${PN} += " \
    libobmc-i2c \
    libsensor-correction \
    "

LDFLAGS += "-lobmc-i2c"
