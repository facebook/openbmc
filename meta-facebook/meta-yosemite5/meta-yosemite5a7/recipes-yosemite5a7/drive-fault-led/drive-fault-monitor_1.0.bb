SUMMARY = "OEM Drive SEL Monitor"
DESCRIPTION = "Watches phosphor-logging for UnifiedSELEvent entries \
               and asserts the corresponding LED fault groups."
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit meson pkgconfig systemd

SYSTEMD_SERVICE:${PN} = "drive-fault-monitor.service"

SRC_URI = "file://drive-fault-monitor.cpp \
           file://drive-fault-monitor.hpp \
           file://meson.build \
           file://drive-fault-monitor.service \
           "

S = "${UNPACKDIR}"

DEPENDS = " \
    sdbusplus \
    phosphor-logging \
    systemd \
"
