FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI += " \
    file://1000-dt-bindings-arm-aspeed-add-Meta-Clemente-board.patch \
    file://1001-ARM-dts-aspeed-clemente-add-Meta-Clemente-BMC.patch \
    file://1002-ARM-dts-aspeed-clemente-add-various-device-nodes-and.patch \
    file://1003-ARM-dts-aspeed-clemente-update-DTS-for-DVT-board.patch \
    file://clemente-sensor.cfg \
"

