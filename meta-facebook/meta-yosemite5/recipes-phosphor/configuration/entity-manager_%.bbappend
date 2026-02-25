FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-configurations-enable-CX7-NIC-automatic-EID-assignme.patch \
"
