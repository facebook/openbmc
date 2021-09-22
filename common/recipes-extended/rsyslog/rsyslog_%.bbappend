FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
PACKAGECONFIG:remove = "gnutls"
DEPENDS:append = " curl libgcrypt"
PTEST_ENABLED = ""
SRC_URI += "file://0001-initd-use-flock-on-start-stop-daemon.patch;patchdir=${WORKDIR} \
"
