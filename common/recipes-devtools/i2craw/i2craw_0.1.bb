LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://i2craw.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://i2craw.c \
    file://Makefile \
    "

LDFLAGS = " -lobmc-i2c -llog "

DEPENDS = " libobmc-i2c liblog "
RDEPENDS:${PN} = " libobmc-i2c liblog "

do_install(){
    install -d ${D}${sbindir}
    install -m 755 i2craw ${D}${sbindir}/i2craw
}
