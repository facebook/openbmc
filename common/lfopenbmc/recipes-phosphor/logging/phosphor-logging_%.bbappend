FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-lg2-commit-Allow-users-to-provide-additional-data.patch \
    file://0002-amd-event-log-add-runtime-AFID-integration.patch \
    file://0003-amd-event-log-add-Redfish-projection-support.patch \
    file://0004-cper-add-trivial-processing-daemon.patch \ 
    file://0005-cper-read-CPER-binary.patch \
    file://0006-cper-decode-add-libcper-decoding.patch \
    file://0007-cper-add-OEM-registration-API.patch \
    file://0008-cper-implement-oem-parser-registry.patch \
"

DEPENDS:append = " \
    libcper \
"

PACKAGECONFIG:append = "cper-processor"

PACKAGECONFIG[cper-processor] = " \
        -Dcper-processor=enabled, \
        -Dcper-processor=disabled, \
        libcper \
"
FILES:${PN}-base:append = " \
        ${@bb.utils.contains('PACKAGECONFIG', 'cper-processor', '${libexecdir}/phosphor-logging/phosphor-cper-processor', '', d)} \
"

DBUS_SERVICE:${PN}-base:append = " \
        ${@bb.utils.contains('PACKAGECONFIG', 'cper-processor', 'xyz.openbmc_project.Logging.CPER.Processor.service', '', d)} \
"

EXTRA_OEMESON:append = " \
  -Damd-event-log=enabled \
  -Dcper-processor=enabled \
  -Druntime-metadata-plugin=xyz.openbmc_project.Logging.Extension.CPER.Processed \
"
