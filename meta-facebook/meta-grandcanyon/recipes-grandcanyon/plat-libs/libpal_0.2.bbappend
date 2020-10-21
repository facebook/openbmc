FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += " \
    file://plat/meson.build \        
"

DEPENDS += " \
    libfbgc-common \
    libfbgc-fruid \
    libobmc-sensors \
    "

# These shouldn't be needed but are because we aren't properly versioning the
# shared libraries contained in these recipes.
RDEPENDS_${PN} += " \
    libfbgc-common \
    libfbgc-fruid \
    libobmc-sensors \
    "
