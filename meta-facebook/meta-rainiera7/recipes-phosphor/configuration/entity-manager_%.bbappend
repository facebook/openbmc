FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-configurations-rainier-Support-for-a-second-source-M.patch \
    file://0002-configurations-revise-100G-BRCM-NIC-configuration.patch \
    file://0003-configurations-revise-Terminus-100G-NIC-configuratio.patch \
    file://0004-configurations-rainier-Remove-CPU-MCTPI3CTarget.patch \
    file://0005-configurations-rainier-Update-FSC-v2-configs.patch \
"
