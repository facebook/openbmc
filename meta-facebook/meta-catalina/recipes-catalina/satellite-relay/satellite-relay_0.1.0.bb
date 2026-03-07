inherit cargo cargo-update-recipe-crates systemd

SUMMARY = "satellite-relay"
HOMEPAGE = "https://github.com/facebook/openbmc"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"

SYSTEMD_SERVICE:${PN} += "xyz.openbmc_project.SatelliteRelay.service"

S = "${UNPACKDIR}"
SRC_URI = " \
	file://Cargo.toml \
	file://Cargo.lock \
	file://LICENSE \
	file://src/dbus.rs \
	file://src/error.rs \
	file://src/lib.rs \
	file://src/main.rs \
	file://src/redfish.rs \
	file://xyz.openbmc_project.SatelliteRelay.service \
"

require ${BPN}-crates.inc

do_install:append() {
	install -d ${D}${systemd_system_unitdir}
	install -m 0644 ${S}/xyz.openbmc_project.SatelliteRelay.service ${D}${systemd_system_unitdir}
}
