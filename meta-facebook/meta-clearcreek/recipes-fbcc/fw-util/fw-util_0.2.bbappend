FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://cpld.cpp \
            file://switch.cpp \
            file://mcu_fw.h \
            file://mcu_fw.cpp \
            file://usbdbg.h \
            file://usbdbg.cpp \
            file://vr_fw.cpp \
            file://vr_fw.h \
            file://nic_mctp.cpp \
            file://nic_mctp.h \
            file://platform.cpp \
           "

DEPENDS += "libfpga libast-jtag libkv libobmc-i2c libmcu libvr libobmc-mctp"
RDEPENDS_${PN} += "libfpga libast-jtag  libkv libobmc-i2c libmcu libvr libobmc-mctp"
LDFLAGS += " -lfpga -last-jtag -lkv -lobmc-i2c -lmcu -lvr -lobmc-mctp"
