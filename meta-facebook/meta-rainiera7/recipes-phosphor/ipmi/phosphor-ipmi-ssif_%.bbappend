FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-Revert-ssifbridged-handle-Get-System-Interface-Capab.patch \
"
