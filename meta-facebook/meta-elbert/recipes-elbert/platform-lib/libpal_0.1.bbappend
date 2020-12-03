FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://pal.c \
            file://pal.h \
            "

DEPENDS += "libgpio libsensor-correction"
RDEPENDS_${PN} += "libgpio libsensor-correction"
LDFLAGS += " -lgpio -lsensor-correction"
