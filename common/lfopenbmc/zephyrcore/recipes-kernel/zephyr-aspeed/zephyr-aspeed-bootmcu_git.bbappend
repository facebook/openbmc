FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0100-boards-ast2700_evb-bootmcu-revise-baudrate-config.patch;patchdir=zephyr \
"
