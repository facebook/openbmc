FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-lg2-commit-Allow-users-to-provide-additional-data.patch \
    file://0002-log_manager-do-not-reset-entryId-on-eraseAll.patch \
    file://0003-amd-event-log-add-runtime-AFID-integration.patch \
    file://0004-amd-event-log-add-Redfish-projection-support.patch \
"
