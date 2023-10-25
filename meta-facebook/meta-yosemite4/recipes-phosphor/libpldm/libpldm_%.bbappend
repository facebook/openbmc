FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

EXTRA_OEMESON:append = " -Doem-meta=enabled"

SRC_URI += " \
    file://0001-Support-OEM-META-write-file-req-decode.patch \
"
