SUMMARY = "At Scale Debug daemon v1.4.7"
DESCRIPTION = "The asd daemon to receive/transmit messages"
SECTION = "base"
PR = "r1"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://LICENSE;beginline=4;endline=25;md5=8861f37222b237b82044f06478647f8b"

SRC_URI = " file://daemon/asd_common.h \
            file://daemon/asd_main.c \
            file://daemon/asd_main.h \
            file://daemon/asd_msg.c \
            file://daemon/asd_msg.h \
            file://daemon/authenticate.c \
            file://daemon/authenticate.h \
            file://daemon/auth_none.c \
            file://daemon/auth_none.h \
            file://daemon/CMakeLists.txt \
            file://daemon/config.c \
            file://daemon/config.h \
            file://daemon/ext_network.c \
            file://daemon/ext_network.h \
            file://daemon/ext_tcp.c \
            file://daemon/ext_tcp.h \
            file://daemon/i2c_handler.c \
            file://daemon/i2c_handler.h \
            file://daemon/i2c_msg_builder.c \
            file://daemon/i2c_msg_builder.h \
            file://daemon/i3c_handler.c \
            file://daemon/i3c_handler.h \
            file://daemon/tests/jtag.h \
            file://daemon/jtag_handler.c \
            file://daemon/jtag_handler.h \
            file://daemon/LICENSE \
            file://daemon/logging.c \
            file://daemon/logging.h \
            file://daemon/session.c \
            file://daemon/session.h \
            file://daemon/target_handler.c \
            file://daemon/target_handler.h \
            file://daemon/vprobe_handler.c \
            file://daemon/vprobe_handler.h \
            file://daemon/pin_handler.c \
          "

DEPENDS += "safec libpal"
RDEPENDS:${PN} += "safec libpal"

S = "${WORKDIR}/daemon"

inherit cmake pkgconfig

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 asd ${dst}/asd
  ln -snf ../fbpackages/${pkgdir}/asd ${bin}/asd
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"
FILES:${PN} = "${FBPACKAGEDIR}/asd ${prefix}/local/bin ${sysconfdir} "
