LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://fruid_util_remote.c;beginline=4;endline=5;md5=6ece31942da90b165e782ec0832a4a95"

SRC_URI  = "file://fruid_util_remote.c \
            file://Makefile \
           "
LDFLAGS += " -lbic "

S = "${WORKDIR}"

DEPENDS += " libbic "
RDEPENDS_${PN} += " libbic "

do_install(){
    install -d ${D}${sbindir}
    install -d ${D}${sysconfdir}
    install -m 755 fruid_util_remote ${D}${sbindir}/fruid_util_remote
}
