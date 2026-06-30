FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-configurations-Add-MCTPI2CTarget-records-for-NIC.patch \
"
