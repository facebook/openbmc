LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://vr_update.c;beginline=1;endline=2;md5=00c470b0510973ef43dacfaf4b849a74"

SRC_URI  = "file://vr_update.c \
            file://Makefile \
           "
LDFLAGS += " -lbic "

S = "${WORKDIR}"

DEPENDS += " libbic "
RDEPENDS_${PN} += " libbic "

do_install(){
    install -d ${D}${sbindir}
    install -d ${D}${sysconfdir}
    install -m 755 vrupdate_poc ${D}${sbindir}/vrupdate_poc
}
