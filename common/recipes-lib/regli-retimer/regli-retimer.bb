DESCRIPTION = "The driver to communicate with Kandou Regli retimer."
SUMMARY = "Regli Retimer Driver"
HOMEPAGE = "https://kandou.com/"
LICENSE = "CLOSED"

S = "${WORKDIR}/sources"
UNPACKDIR="${S}"

SRC_URI = "\
    file://meson.build \
    file://kb900x.h \
    file://kb900x.c \
    file://kb900x_utils.h \
    file://kb900x_utils.c \
    file://kb900x_log.h \
    file://kb900x_log.c \
    file://kb900x_addresses.h \
    file://kb900x_comm.h \
    file://kb900x_comm.c \
    file://plat/kb900x_bic_comm.c \
    file://plat/kb900x_i2c_comm.c \
    "

inherit meson pkgconfig

DEPENDS += ""

RDEPENDS:${PN} += ""
