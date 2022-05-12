# Copyright 2017-present Facebook. All Rights Reserved.

SUMMARY = "At Scale Debug daemon v1.4.3"
DESCRIPTION = "The asd daemon to receive/transmit messages"
SECTION = "base"
PR = "r1"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://LICENSE;beginline=4;endline=25;md5=e98e000db2e37be52014b914cb36eb77"

SRC_URI = " file://daemon/asd_main.c \
            file://daemon/asd_main.h \
            file://daemon/asd_common.h \
            file://daemon/logging.c \
            file://daemon/logging.h \
            file://daemon/ext_network.c \
            file://daemon/ext_network.h \
            file://daemon/authenticate.c \
            file://daemon/authenticate.h \
            file://daemon/session.c \
            file://daemon/session.h \
            file://daemon/config.c \
            file://daemon/config.h \
            file://daemon/jtag_handler.c \
            file://daemon/jtag_handler.h \
            file://daemon/asd_msg.c \
            file://daemon/asd_msg.h \
            file://daemon/ext_tcp.c \
            file://daemon/ext_tcp.h \
            file://daemon/auth_none.c \
            file://daemon/auth_none.h \
            file://daemon/target_handler.c \
            file://daemon/target_handler.h \
            file://daemon/i2c_msg_builder.c \
            file://daemon/i2c_msg_builder.h \
            file://daemon/i2c_handler.c \
            file://daemon/i2c_handler.h \
            file://daemon/mem_helper.c \
            file://daemon/mem_helper.h \
            file://daemon/jtag_test.c \
            file://daemon/jtag_test.h \
            file://daemon/jtag.h \
            file://daemon/pin_handler.c \
            file://daemon/pin_handler.h \
            file://daemon/LICENSE \
            file://daemon/CMakeLists.txt \
          "

DEPENDS = "openssl  \
           libpal \
           "

RDEPENDS:${PN} = "openssl  \
                  libpal \
                 "

S = "${WORKDIR}/daemon"

inherit cmake

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

