LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://i2cupdate.c;beginline=4;endline=16;md5=83b2e504ba70aadc346736cab822cd4c"

SRC_URI  = "file://i2cupdate.c \
            file://max10_i2c_update.c \
            file://max10_i2c_update.h \
            file://Makefile \
           "
LDFLAGS += " -lobmc-i2c "

S = "${WORKDIR}"

DEPENDS += " libobmc-i2c "
RDEPENDS_${PN} += " libobmc-i2c "

do_install(){
    install -d ${D}${sbindir}
    install -d ${D}${sysconfdir}
    install -m 755 cpldupdate_poc ${D}${sbindir}/cpldupdate_poc
}
