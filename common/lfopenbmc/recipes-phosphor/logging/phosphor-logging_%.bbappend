FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-lg2-commit-Allow-users-to-provide-additional-data.patch \
    file://0002-log_manager-do-not-reset-entryId-on-eraseAll.patch \
    file://0003-plugin-Add-metadata-support-to-EventInfo.patch \
    file://0004-plugin-introduce-runtime-interfaces.patch \
    file://0005-plugin-introduce-descriptor-framework.patch \
    file://0006-plugin-introduce-factory-interface.patch \
    file://0007-plugin-add-registry-support.patch \
    file://0008-plugin-add-descriptor-generation-support.patch \
    file://0009-plugin-add-manager-support.patch \
    file://0010-logging-integrate-runtime-plugin-infrastructure.patch \
    file://0011-logging-defer-entry-object-added-signal-emission.patch \
    file://0012-logging-create-log-plugins-from-extension-metadata.patch \
    file://0013-logging-add-plugin-request-extension-support.patch \
    file://0014-plugin-CPER-Processed-Add-extension-support.patch \
    file://0015-logging-add-runtime-metadata-provider-support.patch \
    file://0016-extensions-add-initial-AEL-infrastructure.patch \
    file://0017-amd-event-log-add-AEL-reverse-LUT-codegen-support.patch \
    file://0018-extensions-amd-event-log-add-AEL-runtime-metadata.patch \
    file://0019-config-Add-meson-option-for-runtime-metadata.patch \
    file://0020-plugin-Add-extension-payload-builder-support.patch \
    file://0021-plugin-CPER-Processed-Add-extension-payload-support.patch \
    file://0022-plugin-add-artifact-storage-support.patch \
    file://0023-plugin-CPER-Raw-Add-plugin-support.patch \
    file://0024-amd-event-log-add-common-metadata-support.patch \
    file://0025-amd-event-log-add-Redfish-projection-support.patch \
    file://0026-logging-add-plugin-deletion-lifecycle-support.patch \
    file://0027-plugin-CPER-Raw-Cleanup-artifact-on-entry-deletion.patch \
    file://0028-plugin-add-runtime-plugin-persistence-framework.patch \
    file://0029-plugin-cper-processed-add-persistence-support.patch \
    file://0030-plugin-cper-raw-add-restore-support.patch \
"
