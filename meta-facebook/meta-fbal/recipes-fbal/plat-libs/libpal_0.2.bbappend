FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += "\
    file://pal_cm.c \
    file://pal_cm.h \
    file://pal_ep.c \
    file://pal_ep.h \
    file://pal_health.c \
    file://pal_health.h \
    file://pal_power.c \
    file://pal_power.h \
    file://pal_sbmc.c \
    file://pal_sbmc.h \
    file://pal_sensors.c \
    file://plat/meson.build \
    "

DEPENDS += " \
    libfbal-fruid \
    libgpio-ctrl \
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
    libfbal-fruid \
    libgpio-ctrl \
    libncsi \
    libnl-wrapper \
    libnm \
    libobmc-i2c \
    libobmc-pmbus \
    libobmc-sensors \
    libpeci \
    "
