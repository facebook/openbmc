LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://bic_remote_update.c;beginline=1;endline=2;md5=5e8ec6eeef97b1216595b1edf7eb6a1a"

SRC_URI  = "file://bic_remote_update.c \
            file://Makefile \
           "
LDFLAGS += " -lbic "

S = "${WORKDIR}"

DEPENDS += " libbic "
RDEPENDS_${PN} += " libbic "

do_install(){
    install -d ${D}${sbindir}
    install -d ${D}${sysconfdir}
    install -m 755 bic_remote_update ${D}${sbindir}/bic_remote_update
}
