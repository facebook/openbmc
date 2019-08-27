LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://i2cupdate.c;beginline=4;endline=16;md5=4600fd9f038febc050bcb82abb40d3dd"

SRC_URI  = "file://i2cupdate.c \
            file://max10_i2c_update.c \
            file://max10_i2c_update.h \
            file://Makefile \
           "
LDFLAGS += " -lobmc-i2c -lbic "

S = "${WORKDIR}"

DEPENDS += " libobmc-i2c libbic "
RDEPENDS_${PN} += " libobmc-i2c libbic "

do_install(){
    install -d ${D}${sbindir}
    install -d ${D}${sysconfdir}
    install -m 755 cpldupdate_poc ${D}${sbindir}/cpldupdate_poc
}
