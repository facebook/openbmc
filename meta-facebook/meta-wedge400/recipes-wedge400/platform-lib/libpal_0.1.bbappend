FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += "file://pal.c \
            file://pal.h \
            "

DEPENDS += "libbic libsensor-correction"
RDEPENDS_${PN} += " libbic libsensor-correction"
LDFLAGS += " -lbic -lsensor-correction -lm"
