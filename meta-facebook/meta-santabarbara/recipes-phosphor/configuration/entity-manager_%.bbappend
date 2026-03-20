FILESEXTRAPATHS:prepend := "${THISDIR}/entity-manager:"

SRC_URI:append = " \
    file://0001-configurations-Revise-the-OCP-NIC-sensor-name.patch \
    file://0002-configuration-santabarbara-add-MCTP-I2C-target-for-S.patch \
    file://0003-configuration-santabarbara-configure-MMC-as-MCTPI2CT.patch \
    file://0004-configurations-revise-the-BRCM-800G-OCP-NIC-sensor-n.patch \
"
