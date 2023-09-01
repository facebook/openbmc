FILESEXTRAPATHS:prepend := "${THISDIR}/files/pal:"

CFLAGS:prepend = "-DPLATFORM_NAME="inspirationpoint""
EXTRA_OEMESON:append = " -Dcrashdump-amd=true"

LOCAL_URI += "\
    file://pal_def_cover.h \
    "
