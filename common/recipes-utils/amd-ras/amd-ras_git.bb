SUMMARY = "AMD RAS application to handle RAS errors from BMC"
DESCRIPTION = "The applications harvests and handles the RAS errors from the processor"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit meson pkgconfig systemd

SRC_URI = "git://github.com/AMDESE/amd-bmc-ras.git;branch=integ_sp7;protocol=https \
           file://0001-Refactor-amd-ras-to-use-io_context-to-support-newer-.patch \
           file://0002-Use-zu-for-size_t-formatting-to-fix-build-errors.patch \
           "
SRCREV = "daed620a9da0c5a77aa9350d775628d1d6003d4c"

DEPENDS += " \
    boost \
    apml \
    libcper \
    phosphor-dbus-interfaces \
    phosphor-logging \
    sdbusplus \
    libgpiod \
    nlohmann-json \
    "

SYSTEMD_SERVICE:${PN} = "com.amd.RAS@.service"
FILES:${PN} += "${datadir}/amd-bmc-ras"
