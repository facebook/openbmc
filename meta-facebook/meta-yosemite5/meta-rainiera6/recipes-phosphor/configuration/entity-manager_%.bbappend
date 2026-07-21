FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-configurations-Add-MCTPI2CTarget-records-for-NIC.patch \
    file://0002-configurations-rainier-Add-FSC-configurations.patch \
"
