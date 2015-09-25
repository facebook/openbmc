FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://fw_env.config \
            file://patch-2013.07/0000-u-boot-aspeed-064.patch \
            file://patch-2013.07/0001-u-boot-openbmc.patch \
            file://patch-2013.07/0002-Create-snapshot-of-OpenBMC.patch;striplevel=6 \
           "
do_install_append() {
    if [ -e ${WORKDIR}/fw_env.config ] ; then
        install -d ${D}${sysconfdir}
        install -m 644 ${WORKDIR}/fw_env.config ${D}${sysconfdir}/fw_env.config
    fi
}
