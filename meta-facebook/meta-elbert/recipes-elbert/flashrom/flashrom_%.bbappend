FILESEXTRAPATHS:prepend := "${THISDIR}/flashrom:"

SRC_URI:append = " \
    file://0001-flashrom-GD25LQ256D.patch \
"
