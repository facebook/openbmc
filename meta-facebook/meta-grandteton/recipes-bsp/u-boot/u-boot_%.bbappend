FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://facebook-grandteton_defconfig.append \
            file://fw_env.config.64k \
           "

do_copyfile () {
    bbnote "echo do nothing"
}
addtask copyfile after do_patch before do_configure
