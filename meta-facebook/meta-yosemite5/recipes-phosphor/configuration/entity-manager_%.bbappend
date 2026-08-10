FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-configurations-revise-CX7-NIC-configuration.patch \
    file://0002-configurations-revise-200G-BRCM-NIC-configuration.patch \
    file://0003-configurations-yosemite5-add-PowerState-to-MCTPI2CTa.patch \
    file://0004-configurations-yosemite5-add-scale-to-1kW-paddle-HSC.patch \
    file://0005-configurations-yosemite5-Add-T22-drive-configuration.patch \
"
