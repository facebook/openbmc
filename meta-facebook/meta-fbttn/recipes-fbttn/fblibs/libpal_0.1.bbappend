FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += "file://pal.c \
            file://pal.h \
            "

DEPENDS += "libbic libexp libmctp libfbttn-common libfbttn-fruid libfbttn-sensor libnvme-mi"
RDEPENDS_${PN} += "libbic libexp libmctp libfbttn-common libfbttn-fruid libfbttn-sensor libnvme-mi"
LDFLAGS += " -lbic -lexp -lmctp -lfbttn_common -lfbttn_fruid -lfbttn_sensor -lnvme-mi"
CFLAGS += " -DCONFIG_FBTTN=1"
