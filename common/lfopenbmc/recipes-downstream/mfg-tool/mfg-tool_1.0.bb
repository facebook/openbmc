SUMMARY = "Linux Foundation OpenBMC based mfg-tool utility"
DESCRIPTION = "A wrapper utility for manufacturing use cases"
SECTION = "base"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit meson pkgconfig
S = "${UNPACKDIR}"

LOCAL_URI = " \
    file://meson.build \
    file://config.meson.hpp \
    file://mfg-tool.cpp \
    file://cmd \
    file://utils \
    file://scripts \
    "

DEPENDS += " \
    cli11 \
    nlohmann-json \
    phosphor-dbus-interfaces \
    phosphor-logging \
    sdbusplus \
    "

RDEPENDS:${PN} += " \
    bash \
    phosphor-dbus-interfaces-redfish-registry \
    "

do_install:append() {
    install -m 755 ${UNPACKDIR}/scripts/table-sensor-display ${D}${bindir}/table-sensor-display
    install -m 755 ${UNPACKDIR}/scripts/table-leakdetector-display ${D}${bindir}/table-leakdetector-display
    install -m 755 ${UNPACKDIR}/scripts/table-log-display ${D}${bindir}/table-log-display
    install -m 755 ${UNPACKDIR}/scripts/sort-log-entry ${D}${bindir}/sort-log-entry
}
