FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

LOCAL_URI += " \
    file://plat/meson.build \
    file://cpld.cpp \
    file://mcu_fw.cpp \
    file://mcu_fw.h \
    file://nic_mctp.cpp \
    file://nic_mctp.h \
    file://platform.cpp \
    file://switch.cpp \
    file://switch.h \
    file://usbdbg.cpp \
    file://usbdbg.h \
    file://vr_fw.cpp \
    file://vr_fw.h \
    "

DEPENDS += "libfpga libast-jtag libmcu libvr libobmc-mctp"
