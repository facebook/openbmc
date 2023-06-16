FILESEXTRAPATHS:prepend := "${THISDIR}/files/pal:"

LOCAL_URI += " \
    file://pal_health.c \
    file://pal_health.h \
    file://pal_power.c \
    file://pal_power.h \
    file://pal_sensors.c \
    file://pal_sensors.h \
    file://plat/meson.build \
    "

DEPENDS += " \
    libgpio-ctrl \
    libme \
    libncsi \
    libnl-wrapper \
    libnodemanager \
    libobmc-i2c \
    libobmc-pmbus \
    libobmc-sensors \
    libpeci \
    "

# These shouldn't be needed but are because we aren't properly versioning the
# shared libraries contained in these recipes.
RDEPENDS:${PN} += " \
    libgpio-ctrl \
    libme \
    libnl-wrapper \
    libnodemanager \
    libobmc-i2c \
    libobmc-sensors \
    libpeci \
    "
