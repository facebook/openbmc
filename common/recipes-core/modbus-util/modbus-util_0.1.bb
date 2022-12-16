inherit meson pkgconfig

LOCAL_URI = " \
    file://meson.build \
    file://modbus-util.cpp \
"

LIC_FILES_CHKSUM = " \
    file://modbus-util.cpp;beginline=1;endline=1;md5=5b7429535cf6ba0d7aad6e3019ac2396 \
"

DEPENDS += "libmisc-utils cli11"
RDEPENDS:${PN} = "libmisc-utils"

SUMMARY = "Utility for Modbus debugging"
LICENSE = "GPL-2.0-or-later"
