FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://patch-2013.07/0000-u-boot-aspeed-064.patch \
            file://patch-2013.07/0001-u-boot-openbmc.patch \
            file://patch-2013.07/0002-Create-snapshot-of-OpenBMC.patch \
            "

PV = "v2013.07"

S = "${WORKDIR}/u-boot-v2013.07"

# This is an ugly hack, but we want to get this out the door, rather than
# spending forever fiddling with yocto and bitbake.

do_install_append() {
    if [ -e ${WORKDIR}/fw_env.config ] ; then
        install -d ${D}${sysconfdir}
        install -m 644 ${WORKDIR}/fw_env.config ${D}${sysconfdir}/fw_env.config
    fi
}
