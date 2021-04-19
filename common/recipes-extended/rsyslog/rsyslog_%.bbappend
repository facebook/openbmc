FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
PACKAGECONFIG_remove = "gnutls"
DEPENDS_append = " curl libgcrypt"
PTEST_ENABLED = ""
SRC_URI += "file://0001-initd-use-flock-on-start-stop-daemon.patch;patchdir=${WORKDIR} \
"
