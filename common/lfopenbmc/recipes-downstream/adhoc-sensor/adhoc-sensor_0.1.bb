SUMMARY = "Ad-hoc Sensor Service"
DESCRIPTION = "OpenBMC service providing ad-hoc sensors (0-100%) from file contents using sdbusplus"
SECTION = "base"
PR = "r4"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit systemd meson pkgconfig
S = "${UNPACKDIR}"

LOCAL_URI = " \
    file://meson.build \
    file://meson_options.txt \
    file://adhoc-sensor.cpp \
    file://adhoc-sensor.service \
    file://README.md \
    file://bmc-oem-metric \
    "

DEPENDS += " \
    boost \
    phosphor-dbus-interfaces \
    phosphor-logging \
    sdbusplus \
    systemd \
    "

# Inventory path to associate adhoc sensors with. Empty means discover it at
# runtime; set it only if the discovered chassis is wrong for the platform.
CHASSIS_PATH ??= ""

EXTRA_OEMESON += "-Ddefault-chassis='${CHASSIS_PATH}'"

SYSTEMD_SERVICE:${PN} = "adhoc-sensor.service"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/adhoc-sensor.service \
        ${D}${systemd_system_unitdir}/adhoc-sensor.service

    install -d ${D}${bindir}
    install -m 0755 ${UNPACKDIR}/bmc-oem-metric ${D}${bindir}/bmc-oem-metric
}

FILES:${PN} += "${systemd_system_unitdir}/adhoc-sensor.service"
