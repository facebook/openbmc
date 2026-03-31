FILESEXTRAPATHS:prepend:openbmc-fb-lf := "${THISDIR}/files:"

# EXTRA_OEMESON:append:openbmc-fb-lf = " -Doem-meta=enabled"

SRC_URI:append:openbmc-fb-lf = " \
    file://host_eid \
    file://0001-oem-meta-Add-file-IO-responder-for-event-logs-from-B.patch \
    file://0002-oem-meta-santabarbara-add-handler-for-event-logs-fro.patch \
    file://0003-pldm-Revise-image-path-for-update.patch \
    file://0004-platform-mc-Handle-get-PLDM-Commands-failure.patch \
    file://0005-requester-Validate-MCTP-EID-before-removal.patch \
    file://0006-platform-mc-Add-MCTP-recovery-option.patch \
    file://0007-platform-mc-Add-option-for-discovering-FRU-data.patch \
    file://0008-common-Add-blocking-instance-ID-allocation.patch \
    file://0009-requester-Always-re-initialize-MCTP-info.patch \
    file://0010-platform-mc-Add-retries-for-terminus-discovery.patch \
    file://0011-platform-mc-Make-fallback-terminus-name-optional.patch \
    file://0012-oem-meta-Add-file-IO-responder-for-crashdump-from-BI.patch \
    file://0013-oem-meta-Add-APML-alert-handler.patch \
    file://0014-oem-meta-Add-MCTP-configuration-discovery.patch \
    file://0015-pldm-Increase-maximum-of-dbus-timeout-value.patch \
    file://0016-terminus-replace-inventory-and-sensor-configuration.patch \
    file://0017-requester-refresh-MCTP-endpoints-after-host-reaches-.patch \
    file://0018-oem-meta-Add-call-to-the-fw-versions-sd-retimer-serv.patch \
    file://0019-oem-meta-add-getSlotNumberStringByTID-mapping-for-Sa.patch \
"

SYSTEMD_AUTO_ENABLE:${PN}:openbmc-fb-lf = "enable"

do_install:append:openbmc-fb-lf() {
    install -d ${D}/usr/share/pldm
    install -m 0444 ${UNPACKDIR}/host_eid ${D}/usr/share/pldm
}

