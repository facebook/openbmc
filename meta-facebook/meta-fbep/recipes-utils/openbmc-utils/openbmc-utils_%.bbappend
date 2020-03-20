FILESEXTRAPATHS_append := "${THISDIR}/files:"

do_install_append(){
    # auto-mount emmc to /mnt/data1
    install -m 0755 ${WORKDIR}/mount_data1.sh ${D}${sysconfdir}/init.d/mount_data1.sh
    update-rc.d -r ${D} mount_data1.sh start 05 S .
}
