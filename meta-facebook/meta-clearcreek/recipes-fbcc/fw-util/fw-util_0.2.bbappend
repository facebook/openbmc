FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://cpld.cpp \
           "

DEPENDS += "libfpga libast-jtag libkv libobmc-i2c"
RDEPENDS_${PN} += "libfpga libast-jtag  libkv libobmc-i2c"
LDFLAGS += " -lfpga -last-jtag -lkv -lobmc-i2c"