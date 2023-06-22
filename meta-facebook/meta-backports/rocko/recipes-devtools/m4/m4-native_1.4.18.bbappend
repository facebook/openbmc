FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://m4-1.4.18-glibc-change-work-around.patch \
            file://011-fix-sigstksz.patch \
           "
