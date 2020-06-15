FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += " \
    file://plat/meson.build \
    "

DEPENDS += " \
    libbic \
    libminilaketb-common \
    libminilaketb-fruid \
    libminilaketb-sensor \
    "

# These shouldn't be needed but are because we aren't properly versioning the
# shared libraries contained in these recipes.
RDEPENDS_${PN} +=  " \
    libbic \
    libminilaketb-common \
    libminilaketb-fruid \
    libminilaketb-sensor \
    "
