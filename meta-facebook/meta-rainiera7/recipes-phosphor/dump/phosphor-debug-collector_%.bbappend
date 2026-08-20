FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-faultlog-Use-PrimaryLogId-as-faultlog-file-for-CPER.patch \
"

DEPENDS:append = " \
    libcper \
"