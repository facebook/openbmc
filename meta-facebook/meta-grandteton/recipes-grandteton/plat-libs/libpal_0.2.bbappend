FILESEXTRAPATHS:prepend := "${THISDIR}/files/pal:"

LOCAL_URI += "\
    file://pal_health.c \
    file://pal_health.h \
    file://pal_power.c \
    file://pal_power.h \
    file://pal_cfg.c \
    file://pal_cfg.h \
    file://pal_common.c \
    file://pal_common.h \
    file://pal_def.h \
    file://plat/meson.build \
    "

DEPENDS += " \
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
RDEPENDS:${PN} += " \
    libgpio-ctrl \
    libncsi \
    libnl-wrapper \
    libnm \
    libobmc-i2c \
    libobmc-pmbus \
    libobmc-sensors \
    libpeci \
    "
