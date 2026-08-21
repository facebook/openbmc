FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = " \
    file://0001-platform-use-uint32-for-CPER-event-data-length.patch \
"