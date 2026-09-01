FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-lg2-commit-Allow-users-to-provide-additional-data.patch \
    file://0002-log_manager-do-not-reset-entryId-on-eraseAll.patch \
    file://0003-log-create-add-extend-to-attach-extension-data-to-ev.patch \
    file://0004-test-openpower-pels-sync-extension-build-settings.patch \
    file://0005-lg2-replace-extractEvent-tuple-with-EventInfo.patch \
    file://0006-plugin-Add-metadata-support-to-EventInfo.patch \
    file://0007-plugin-introduce-runtime-interfaces.patch \
    file://0008-plugin-introduce-descriptor-framework.patch \
    file://0009-plugin-introduce-factory-interface.patch \
    file://0010-plugin-add-registry-support.patch \
    file://0011-plugin-add-descriptor-generation-support.patch \
    file://0012-plugin-add-manager-support.patch \
    file://0013-logging-integrate-runtime-plugin-infrastructure.patch \
    file://0014-logging-defer-entry-object-added-signal-emission.patch \
    file://0015-logging-create-log-plugins-from-extension-metadata.patch \
    file://0016-logging-add-plugin-request-extension-support.patch \
    file://0017-plugin-CPER-Processed-Add-extension-support.patch \
    file://0018-logging-add-runtime-metadata-provider-support.patch \
    file://0019-extensions-add-initial-AEL-infrastructure.patch \
    file://0020-amd-event-log-add-AEL-reverse-LUT-codegen-support.patch \
    file://0021-extensions-amd-event-log-add-AEL-runtime-metadata.patch \
    file://0022-config-Add-meson-option-for-runtime-metadata.patch \
    file://0023-plugin-Add-extension-payload-builder-support.patch \
    file://0024-plugin-CPER-Processed-Add-extension-payload-support.patch \
    file://0025-plugin-add-artifact-storage-support.patch \
    file://0026-plugin-CPER-Raw-Add-plugin-support.patch \
    file://0027-amd-event-log-add-common-metadata-support.patch \
    file://0028-amd-event-log-add-Redfish-projection-support.patch \
    file://0029-logging-add-plugin-deletion-lifecycle-support.patch \
    file://0030-plugin-CPER-Raw-Cleanup-artifact-on-entry-deletion.patch \
    file://0031-plugin-add-runtime-plugin-persistence-framework.patch \
    file://0032-plugin-cper-processed-add-persistence-support.patch \
    file://0033-plugin-cper-raw-add-restore-support.patch \
"
