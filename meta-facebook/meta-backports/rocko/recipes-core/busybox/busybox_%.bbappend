FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

# The patch is for bug discovered on rocko which busybox version is 1.24.1.
# The patch has been included in newer version than 1.24.1.
SRC_URI += " \
            file://fix-uninitialized-memory-when-displaying-IPv6.patch \
           "
