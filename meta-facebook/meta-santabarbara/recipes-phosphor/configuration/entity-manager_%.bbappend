FILESEXTRAPATHS:prepend := "${THISDIR}/entity-manager:"

SRC_URI:append = " \
    file://0001-configurations-Revise-the-OCP-NIC-sensor-name.patch \
    file://0002-configuration-santabarbara-add-the-inventory-name-fo.patch \
    file://0003-configurations-revise-the-BRCM-800G-OCP-NIC-sensor-n.patch \
    file://0004-configuration-santabarbara-update-NIC-thresholds.patch \
    file://0005-configuration-santabarbara-add-PLDM-sensors-to-SWB-M.patch \
    file://0006-configuration-santabarbara-add-Inventory-to-Arkes-MM.patch \
    file://0007-configurations-santabarbara-add-MCTPI2CTarget-for-SW.patch \
"
