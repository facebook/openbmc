FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://fbgcvboot2_defconfig.append \
           "

do_copyfile () {
    bbnote "echo do nothing"
}
addtask copyfile after do_patch before do_configure
