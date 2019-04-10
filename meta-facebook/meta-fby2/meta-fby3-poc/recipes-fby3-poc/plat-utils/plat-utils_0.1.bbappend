FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

do_install_append(){
    #remove the unnecessary scripts
    update-rc.d -f -r ${D} setup-sysconfig.sh remove    
    update-rc.d -f -r ${D} setup-server-type.sh remove
    update-rc.d -f -r ${D} setup-platform.sh remove
}
