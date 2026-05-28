FILESEXTRAPATHS:prepend:openbmc-fb-lf := "${THISDIR}/files:"

# EXTRA_OEMESON:append:openbmc-fb-lf = " -Doem-meta=enabled"

SRC_URI:append:openbmc-fb-lf = " \
    file://host_eid \
    file://0001-pldm-Revise-image-path-for-update.patch \
    file://0002-platform-mc-Handle-get-PLDM-Commands-failure.patch \
    file://0003-requester-Validate-MCTP-EID-before-removal.patch \
    file://0004-platform-mc-Add-MCTP-recovery-option.patch \
    file://0005-platform-mc-Add-option-for-discovering-FRU-data.patch \
    file://0006-common-Add-blocking-instance-ID-allocation.patch \
    file://0007-requester-Always-re-initialize-MCTP-info.patch \
    file://0008-platform-mc-Add-retries-for-terminus-discovery.patch \
    file://0009-platform-mc-Make-fallback-terminus-name-optional.patch \
    file://0010-oem-meta-Add-file-IO-responder-for-crashdump-from-BI.patch \
    file://0011-oem-meta-Add-APML-alert-handler.patch \
    file://0012-oem-meta-Add-MCTP-configuration-discovery.patch \
    file://0013-pldm-Increase-maximum-of-dbus-timeout-value.patch \
    file://0014-terminus-replace-inventory-and-sensor-configuration.patch \
    file://0015-terminus-add-refreshFirmwareParameters-method.patch \
    file://0016-requester-refresh-MCTP-endpoints-after-host-reaches-.patch \
    file://0017-oem-meta-add-getSlotNumberStringByTID-mapping-for-Sa.patch \
    file://0018-oem-meta-Implement-special-event-handling.patch \
    file://0019-oem-meta-Add-retry-event-deduplication-for-unified-B.patch \
    file://0020-oem-meta-Add-call-to-the-fw-versions-sd-retimer-serv.patch \
    file://0021-Add-back-sensor-polling-time-configuration.patch \
    file://0022-Add-back-maximum-transfer-size-configuration.patch \
    file://0023-Add-back-instance-id-expiration-interval.patch \
    file://0024-fw_update-Reimplement-package-parser-to-use-new-libp.patch \
    file://0025-platform-mc-lazy-attach-FRU-decorator-interfaces.patch \
"

SYSTEMD_AUTO_ENABLE:${PN}:openbmc-fb-lf = "enable"

do_install:append:openbmc-fb-lf() {
    install -d ${D}/usr/share/pldm
    install -m 0444 ${UNPACKDIR}/host_eid ${D}/usr/share/pldm
}

