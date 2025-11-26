SUMMARY = "Ad-hoc Sensor Service"
DESCRIPTION = "OpenBMC service providing ad-hoc sensors (0-100%) from file contents using sdbusplus"
SECTION = "base"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit systemd meson pkgconfig

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

LOCAL_URI = " \
    file://meson.build \
    file://meson_options.txt \
    file://adhoc-sensor.cpp \
    file://adhoc-sensor.service \
    file://README.md \
    "

DEPENDS += " \
    boost \
    phosphor-dbus-interfaces \
    phosphor-logging \
    sdbusplus \
    systemd \
    "

# Default chassis path - MUST be overridden in platform-specific bbappend
CHASSIS_PATH ??= "/xyz/openbmc_project/inventory/system/chassis"

EXTRA_OEMESON += "-Ddefault-chassis='${CHASSIS_PATH}'"

SYSTEMD_SERVICE:${PN} = "adhoc-sensor.service"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/adhoc-sensor.service \
        ${D}${systemd_system_unitdir}/adhoc-sensor.service
}

FILES:${PN} += "${systemd_system_unitdir}/adhoc-sensor.service"
