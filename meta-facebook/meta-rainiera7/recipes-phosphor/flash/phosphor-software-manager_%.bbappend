FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-i2cvr-cache-VR-version-to-allow-read-access-when-hos.patch \
"
