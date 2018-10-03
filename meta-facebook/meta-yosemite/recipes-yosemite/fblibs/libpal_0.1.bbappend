FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += "file://pal.c \
            file://pal.h \
            "

DEPENDS += "libbic libyosemite-common libyosemite-fruid libyosemite-sensor"
RDEPENDS_${PN} += " libbic libyosemite-common libyosemite-fruid libyosemite-sensor"
LDFLAGS += " -lbic -lyosemite_common -lyosemite_fruid -lyosemite_sensor"
