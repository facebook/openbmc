FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

#
# Use latest i3c hub driver from local folder for now,
# until DTS is updated in other projects already using it.
#
SRC_URI:remove = " \
    file://0209-dt-bindings-i3c-hub-Add-Renesas-RG3MxxB12A1-I3C-HUB.patch \
    file://0210-i3c-i3c-hub-Add-Renesas-RG3MxxB12A1-I3C-HUB-driver.patch \
"

SRC_URI:append = " \
    file://1000-ARM-dts-aspeed-santabarbara-add-lpc_pcc-node.patch \
    file://1001-ARM-dts-aspeed-santabarbara-add-device-nodes-behind-.patch \
    file://1002-ARM-dts-aspeed-santabarbara-configure-dimm-ids-for-s.patch \
    file://1003-enable-uart-dma.patch \
    file://1004-ARM-dts-aspeed-santabarbara-Add-system-monitoring-GP.patch \
    file://1005-ARM-dts-aspeed-santabarbara-Disable-power-monitor-no.patch \
    file://1006-ARM-dts-aspeed-santabarbara-Add-JTAG-GPIO-line-names.patch \
    file://1007-ARM-dts-aspeed-santabarbara-Add-SGPIO-line-names.patch \
    file://1008-ARM-dts-aspeed-santabarbara-Add-leak-cable-present-I.patch \
"

SRC_URI:append = " \
    file://0202-dt-bindings-i3c-Add-i3c-hub-support.patch \
    file://0203-i3c-Add-driver-for-i3c-hub-device.patch \
    file://0204-i3c-i3c-hub-Fix-SMBus-Agent-tx-timeout-issue.patch \
    file://0205-i3c-i3c-hub-Fix-SMBus-Agent-Rx-buf-id-mismatch.patch \
    file://0206-i3c-i3c-hub-Improve-SMBus-Agent-reset-function.patch \
    file://0207-i3c-i3c-hub-Improve-SMBus-Agent-buf-id-sync.patch \
    file://0208-i3c-i3c-hub-Check-VIOS-power-good-status.patch \
    file://0209-i3c-i3c-hub-add-support-for-Realtek-i3c-hub-device-i.patch \
    file://santabarbara-local.cfg \
"
