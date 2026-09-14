FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-lg2-commit-Allow-users-to-provide-additional-data.patch \
    file://0002-log_manager-do-not-reset-entryId-on-eraseAll.patch \
    file://0003-event_extensions-Add-event-extension-framework.patch \
    file://0004-logging-defer-entry-object-added-signal-emission.patch \
    file://0005-event_extensions-Add-persistence-support.patch \
    file://0006-event_extensions-CPER-Add-Processed-extension.patch \
    file://0007-event_extensions-add-artifact-storage-support.patch \
    file://0008-event_extensions-CPER-Add-Raw-extension.patch \
    file://0009-logging-add-runtime-extension-enrichment-support.patch \
    file://0010-amd-event-log-Add-AFID-lookup-infrastructure.patch \
    file://0011-amd-event-log-add-runtime-AFID-integration.patch \
    file://0012-amd-event-log-add-Redfish-projection-support.patch \
"
