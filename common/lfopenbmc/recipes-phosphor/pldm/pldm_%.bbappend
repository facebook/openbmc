FILESEXTRAPATHS:prepend:openbmc-fb-lf := "${THISDIR}/files:"

# EXTRA_OEMESON:append:openbmc-fb-lf = " -Doem-meta=enabled"

SRC_URI:append:openbmc-fb-lf = " \
    file://host_eid \
    file://0001-pldm-Revise-image-path-for-update.patch \
    file://0002-platform-mc-Handle-get-PLDM-Commands-failure.patch \
    file://0003-platform-mc-Add-MCTP-recovery-option.patch \
    file://0004-platform-mc-Add-option-for-discovering-FRU-data.patch \
    file://0005-common-Add-blocking-instance-ID-allocation.patch \
    file://0006-platform-mc-Add-retries-for-terminus-discovery.patch \
    file://0007-platform-mc-Make-fallback-terminus-name-optional.patch \
    file://0008-oem-meta-Add-file-IO-responder-for-crashdump-from-BI.patch \
    file://0009-oem-meta-Add-APML-alert-handler.patch \
    file://0010-oem-meta-Add-MCTP-configuration-discovery.patch \
    file://0011-pldm-Increase-maximum-of-dbus-timeout-value.patch \
    file://0012-terminus-replace-inventory-and-sensor-configuration.patch \
    file://0013-terminus-add-refreshFirmwareParameters-method.patch \
    file://0014-requester-refresh-MCTP-endpoints-after-host-reaches-.patch \
    file://0015-requester-Parse-MCTP-endpoint-from-InterfacesRemoved.patch \
    file://0016-oem-meta-Implement-special-event-handling.patch \
    file://0017-oem-meta-Add-retry-event-deduplication-for-unified-B.patch \
    file://0018-oem-meta-Add-call-to-the-fw-versions-sd-retimer-serv.patch \
    file://0019-Add-back-sensor-polling-time-configuration.patch \
    file://0020-Add-back-maximum-transfer-size-configuration.patch \
    file://0021-Add-back-instance-id-expiration-interval.patch \
    file://0022-fw_update-Reimplement-package-parser-to-use-new-libp.patch \
    file://0023-oem-meta-support-MCTP-I2C-and-I3C-target-configs.patch \
    file://0024-platform-mc-fix-dangling-exec-async_scope-in-doSenso.patch \
    file://0025-oem-meta-santabarbara-update-event-logs-from-rainbow.patch \
"

SYSTEMD_AUTO_ENABLE:${PN}:openbmc-fb-lf = "enable"

do_install:append:openbmc-fb-lf() {
    install -d ${D}/usr/share/pldm
    install -m 0444 ${UNPACKDIR}/host_eid ${D}/usr/share/pldm
}

