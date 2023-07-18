SUMMARY = "At Scale Debug daemon v1.5.1"
DESCRIPTION = "The asd daemon to receive/transmit messages"
SECTION = "base"
PR = "r1"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://LICENSE;beginline=4;endline=25;md5=8861f37222b237b82044f06478647f8b"

SRC_URI = " file://include \
            file://jtag_test \
            file://server \
            file://CMakeLists.txt \
            file://LICENSE \
            file://target/asd_msg.c \
            file://target/asd_msg.h \
            file://target/asd_server_interface.c \
            file://target/asd_target_api.c \
            file://target/CMakeLists.txt \
            file://target/i2c_handler.c \
            file://target/i2c_handler.h \
            file://target/i2c_handler_stub.c \
            file://target/i2c_msg_builder.c \
            file://target/i2c_msg_builder.h \
            file://target/i2c_msg_builder_stub.c \
            file://target/i3c_handler.c \
            file://target/i3c_handler.h \
            file://target/i3c_handler_stub.c \
            file://target/jtag_handler.c \
            file://target/jtag_handler.h \
            file://target/pin_handler.c \
            file://target/platform.c \
            file://target/target_handler.c \
            file://target/target_handler.h \
            file://target/tests \
            file://target/vprobe_handler.c \
            file://target/vprobe_handler.h \
          "

DEPENDS += "safec libasd-jtagintf libpal"
RDEPENDS:${PN} += "safec libasd-jtagintf libpal"
EXTRA_OECMAKE += "-DCMAKE_SKIP_RPATH=TRUE"

S = "${WORKDIR}"

inherit cmake pkgconfig

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 server/asd ${dst}/asd
  ln -snf ../fbpackages/${pkgdir}/asd ${bin}/asd

  install -m 755 jtag_test/jtag_test ${dst}/jtag_test
  ln -snf ../fbpackages/${pkgdir}/jtag_test ${bin}/jtag_test
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"
FILES:${PN} = "${FBPACKAGEDIR}/asd ${FBPACKAGEDIR}/jtag_test ${prefix}/local/bin ${sysconfdir} "
