LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://cpld_update.c;beginline=1;endline=2;md5=00c470b0510973ef43dacfaf4b849a74"

SRC_URI  = "file://cpld_update.c \
            file://Makefile \
           "
LDFLAGS += " -lbic "

S = "${WORKDIR}"

DEPENDS += " libbic "
RDEPENDS_${PN} += " libbic "

do_install(){
    install -d ${D}${sbindir}
    install -d ${D}${sysconfdir}
    install -m 755 m2cpldupdate_poc ${D}${sbindir}/m2cpldupdate_poc
}
