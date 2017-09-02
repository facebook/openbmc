SUMMARY = "lm_sensors configuration files"
DESCRIPTION = "Hardware health monitoring configuration files"
HOMEPAGE = "http://www.lm-sensors.org/"
LICENSE = "MIT-X"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

PACKAGE_ARCH = "${MACHINE_ARCH}"

SRC_URI = "file://fancontrol \
           file://sensors.conf \
"
S = "${WORKDIR}"

RDEPENDS_${PN}-dev = ""

do_install() {
    # Install fancontrol configuration file
    install -d ${D}${sysconfdir}/sysconfig
    install -m 0644 ${WORKDIR}/fancontrol ${D}${sysconfdir}
    # Install libsensors configuration file
    install -d ${D}${sysconfdir}/sensors.d
    install -m 0644 ${WORKDIR}/sensors.conf ${D}${sysconfdir}/sensors.d
}

# libsensors configuration
PACKAGES =+ "${PN}-libsensors"

# fancontrol script configuration
PACKAGES =+ "${PN}-fancontrol"

# libsensors configuration file
FILES_${PN}-libsensors = "${sysconfdir}/sensors.d/sensors.conf"

# fancontrol script configuration file
FILES_${PN}-fancontrol = "${sysconfdir}/fancontrol"
