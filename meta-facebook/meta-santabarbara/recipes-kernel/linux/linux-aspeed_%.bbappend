FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

#
# Use latest i3c hub driver from local folder for now,
# until DTS is updated in other projects already using it.
#
SRC_URI:remove = " \
    file://0202-dt-bindings-i3c-hub-Add-Renesas-RG3MxxB12A1-I3C-HUB.patch \
    file://0203-i3c-i3c-hub-Add-Renesas-RG3MxxB12A1-I3C-HUB-driver.patch \
"

SRC_URI:append = " \
    file://1000-ARM-dts-aspeed-santabarbara-add-lpc_pcc-node.patch \
    file://1001-ARM-dts-aspeed-santabarbara-Add-blank-lines-between-.patch \
    file://1002-ARM-dts-aspeed-santabarbara-Add-sensor-support-for-e.patch \
    file://1003-ARM-dts-aspeed-santabarbara-Enable-MCTP-for-frontend.patch \
    file://1004-ARM-dts-aspeed-santabarbara-Add-bmc_ready_noled-Led.patch \
    file://1005-ARM-dts-aspeed-santabarbara-Add-gpio-line-name.patch \
    file://1006-ARM-dts-aspeed-santabarbara-Add-AMD-APML-interface-s.patch \
    file://1007-ARM-dts-aspeed-santabarbara-Add-eeprom-device-node-f.patch \
    file://1008-ARM-dts-aspeed-santabarbara-add-device-nodes-behind-.patch \
    file://1009-ARM-dts-aspeed-sb-configure-dimm-ids-for-sbrmi.patch \
    file://1010-ARM-dts-aspeed-santabarbara-Add-swb-IO-expander-and-.patch \
    file://1011-ARM-dts-aspeed-santabarbara-Enable-ipmb-device-for-O.patch \
    file://0202-dt-bindings-i3c-Add-i3c-hub-support.patch \
    file://0203-i3c-Add-driver-for-i3c-hub-device.patch \
    file://santabarbara-local.cfg \
"
