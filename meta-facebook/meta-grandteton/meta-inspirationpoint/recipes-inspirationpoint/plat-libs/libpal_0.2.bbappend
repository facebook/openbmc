FILESEXTRAPATHS:prepend := "${THISDIR}/files/pal:"

CFLAGS:prepend = "-DPLATFORM_NAME="inspirationpoint""

LOCAL_URI += "\
    file://pal_def_cover.h \
    "
