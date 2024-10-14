FILESEXTRAPATHS:prepend := "${THISDIR}/linux-nuvoton:${THISDIR}/../../../recipes-kernel/linux/linux-aspeed:"

# Common driver
SRC_URI:append = " file://0501-hwmon-ina233-Add-ina233-driver.patch"
SRC_URI:append = " file://0502-Add-adm1281-driver.patch"
SRC_URI:append = " file://0513-hwmon-Driver-for-Nuvoton-NCT7363Y.patch"
SRC_URI:append = " file://0516-i3c-master-add-enable-disable-hot-join-in-sys-entry.patch"
SRC_URI:append = " file://0519-dt-bindings-i3c-hub-Add-Renesas-RG3MxxB12A1-I3C-HUB.patch"
SRC_URI:append = " file://0520-i3c-i3c-hub-Add-Renesas-RG3MxxB12A1-I3C-HUB-driver.patch"
SRC_URI:append = " file://0531-hwmon-new-driver-for-ISL28022-power-monitor.patch"

# NPCM8XX common dts
SRC_URI:append = " file://1000-dt-bindings-clock-npcm845-Add-reference-25m-clock-pr.patch"
SRC_URI:append = " file://1001-arm64-dts-modify-clock-property-in-modules-node.patch"
SRC_URI:append = " file://1002-arm64-dts-npmc8xx-move-the-clk-handler-node-to-the-r.patch"
SRC_URI:append = " file://1003-arm64-dts-nuvoton-npcm8xx-add-pin-and-gpio-controller-nodes.patch"
SRC_URI:append = " file://1004-arm64-dts-nuvoton-npcm8xx-add-modules-node.patch"

# NPCM8XX YV4 dts
SRC_URI:append = " file://1005-arm64-dts-nuvoton-add-initial-yosemitev4-device-tree.patch"

# NPCM8XX NCSI driver
SRC_URI:append = " file://1015-net-stmmac-Add-NCSI-support-for-STMMAC.patch"

# NPCM8XX FIU driver
SRC_URI:append = " file://1016-spi-npcm-fiu-add-dual-and-quad-write-support.patch"
SRC_URI:append = " file://1017-spi-npcm-fiu-add-delay-before-reboot-to-avoi.patch"

# NPCM8XX PSPI driver
SRC_URI:append = " file://1018-spi-npcm-pspi-Add-full-duplex-support.patch"
SRC_URI:append = " file://1019-spi-npcm-pspi-Fix-transfer-bits-per-word-issue-389.patch"

# NPCM8XX UDC driver
SRC_URI:append = " file://1020-usb-chipidea-udc-enforce-write-to-the-memory.patch"

# NPCM8XX i2c driver
SRC_URI:append = " file://1021-i2c-npcm-disable-interrupt-enable-bit-before-devm_re.patch"
SRC_URI:append = " file://1022-i2c-npcm-use-i2c-frequency-table.patch"
SRC_URI:append = " file://1023-i2c-npcm-use-a-software-flag-to-indicate-a-BER-condi.patch"
SRC_URI:append = " file://1024-i2c-npcm-Modify-timeout-evaluation-mechanism.patch"
SRC_URI:append = " file://1025-i2c-npcm-Modify-the-client-address-assignment.patch"
SRC_URI:append = " file://1026-i2c-npcm7xx.c-Enable-slave-in-eob-interrupt.patch"
SRC_URI:append = " file://1027-i2c-npcm-correct-the-read-write-operation-procedure.patch"
SRC_URI:append = " file://1028-i2c-npcm-Return-EAGAIN-while-bus-is-busy.patch"

# NPCM8XX Jtag Master driver
SRC_URI:append = " file://1030-misc-npcm8xx-jtag-master-Add-NPCM845-JTAG-master-dri.patch"

# NPCM8XX i3c driver
SRC_URI:append = " file://1035-i3c-master-Add-Nuvoton-npcm845-i3c-master-driver.patch"
SRC_URI:append = " file://1036-dt-bindings-i3c-Add-NPCM845-i3c-controller.patch"

# NPCM8XX WDT driver
SRC_URI:append = " file://1040-driver-watchdog-npcm-add-a-support-to-get-reset-casu.patch"

SRC_URI:append = " file://1042-i3c-expose-interface-for-doing-daa-request.patch"
