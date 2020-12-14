FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += " \
    file://plat/meson.build \
    file://pal_sensors.c \
    file://pal_power.c \
    "
DEPENDS += " \
    libfbgc-common \
    libfbgc-fruid \
    libobmc-sensors \
    libobmc-i2c \
    libgpio-ctrl \
    libfbgc-gpio \
    libexp \
    libbic \
    "
# These shouldn't be needed but are because we aren't properly versioning the
# shared libraries contained in these recipes.
RDEPENDS_${PN} += " \
    libfbgc-common \
    libfbgc-fruid \
    libobmc-sensors \
    libobmc-i2c \
    libgpio-ctrl \
    libfbgc-gpio \
    libexp \
    libbic \
    "
