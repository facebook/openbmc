FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-dump-Add-system-dump-entry-class.patch \
    file://0002-dump-Add-system-dump-Manager.patch \
    file://0003-dump-collectors-Add-system-dump-collector-hooks.patch \
    file://0004-dump-collectors-Add-PLDM-system-dump-collector.patch \
"
