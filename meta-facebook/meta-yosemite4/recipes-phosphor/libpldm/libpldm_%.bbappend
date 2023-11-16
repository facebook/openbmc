FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

PACKAGECONFIG = "abi-development"

EXTRA_OEMESON:append = " -Doem-meta=enabled"

SRC_URI += " \
    file://0001-Support-OEM-META-write-file-req-decode.patch \
"
