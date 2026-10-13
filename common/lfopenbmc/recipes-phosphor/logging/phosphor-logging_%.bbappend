FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-lg2-commit-Allow-users-to-provide-additional-data.patch \
    file://0002-amd-event-log-add-runtime-AFID-integration.patch \
    file://0003-amd-event-log-add-Redfish-projection-support.patch \
"
