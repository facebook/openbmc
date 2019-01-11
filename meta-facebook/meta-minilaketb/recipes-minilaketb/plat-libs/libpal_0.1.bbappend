FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += "file://pal.c \
            file://pal.h \
            "

DEPENDS += "libbic libminilaketb-common libminilaketb-fruid libminilaketb-sensor"
RDEPENDS_${PN} += "libbic libminilaketb-common libminilaketb-fruid libminilaketb-sensor"
LDFLAGS += " -lbic -lminilaketb_common -lminilaketb_fruid -lminilaketb_sensor"
