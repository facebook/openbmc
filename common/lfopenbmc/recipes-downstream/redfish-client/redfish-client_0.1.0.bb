inherit pkgconfig meson
inherit ptest-meson
inherit python3native
inherit systemd

SUMMARY = "redfish-client"
HOMEPAGE = "https://github.com/facebook/openbmc"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"
DEPENDS += " \
    ${PYTHON_PN}-mako-native \
    ${PYTHON_PN}-inflection-native \
    boost \
    cli11 \
    curl \
    nlohmann-json \
    phosphor-dbus-interfaces \
    phosphor-logging \
    redfish-schema-pack-native \
    sdbusplus \
    ${@bb.utils.contains('PTEST_ENABLED', '1', 'gtest', '', d)} \
"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

SYSTEMD_SERVICE:${PN} += "xyz.openbmc_project.RedfishClient.service"
SRC_URI = " \
    file://LICENSE \
    file://meson.build \
    file://meson.options \
    file://configurations \
    file://redfish-binding \
    file://test \
    file://async_http_client.cpp \
    file://async_http_client.hpp \
    file://daemon.cpp \
    file://daemon.hpp \
    file://daemon_main.cpp \
    file://log_service_handler.cpp \
    file://log_service_handler.hpp \
    file://persist_map.cpp \
    file://persist_map.hpp \
    file://sensor.cpp \
    file://sensor.hpp \
    file://xyz.openbmc_project.RedfishClient.service \
"

EXTRA_OEMESON = " \
    -Dtests=${@bb.utils.contains('PTEST_ENABLED', '1', 'enabled', 'disabled', d)} \
"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/xyz.openbmc_project.RedfishClient.service ${D}${systemd_system_unitdir}
}
