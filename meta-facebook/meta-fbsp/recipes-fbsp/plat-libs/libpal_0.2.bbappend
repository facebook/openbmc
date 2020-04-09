FILESEXTRAPATHS_prepend := "${THISDIR}/files/:"

SRC_URI += " \
    file://plat/meson.build \
    file://plat/pal.c \
    file://plat/pal.h \
    file://plat/pal_power.c \
    file://plat/pal_power.h \
    file://plat/pal_sensors.c \
    file://plat/pal_sensors.h \
    file://plat/pal_health.c \
    file://plat/pal_health.h \
    "

DEPENDS += " \
    libgpio-ctrl \
    libme \
    libncsi \
    libnl-wrapper \
    libnm \
    libobmc-i2c \
    libobmc-pmbus \
    libobmc-sensors \
    libpeci \
    "

# These shouldn't be needed but are because we aren't properly versioning the
# shared libraries contained in these recipes.
RDEPENDS_${PN} += " \
    libgpio-ctrl \
    libme \
    libnl-wrapper \
    libnm \
    libobmc-i2c \
    libobmc-sensors \
    libpeci \
    "
