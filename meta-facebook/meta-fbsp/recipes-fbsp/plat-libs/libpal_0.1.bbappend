FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += "file://pal.c \
            file://pal.h \
           "

DEPENDS += " libgpio-ctrl"
RDEPENDS_${PN} += " libgpio-ctrl"
LDFLAGS += "-lgpio-ctrl"
