FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://init \
            file://sshd_config \
           "

PR .= ".3"

RDEPENDS_${PN} += "bash"

do_configure_append() {
  sed -ri "s/__OPENBMC_VERSION__/${OPENBMC_VERSION}/g" sshd_config
}
