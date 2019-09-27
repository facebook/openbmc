FILESEXTRAPATHS_append := "${THISDIR}/files:"

do_install_append(){
    #Install the eMMC script
    install -m 0755 ${WORKDIR}/setup_emmc.sh ${D}${sysconfdir}/init.d/setup_emmc.sh
    update-rc.d -r ${D} setup_emmc.sh start 05 S .
}
