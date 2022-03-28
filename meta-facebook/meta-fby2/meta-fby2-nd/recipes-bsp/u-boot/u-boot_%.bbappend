FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://fby2_ext.h \
            file://fby2_defconfig.append \
           "
SRC_URI:remove:mf-tpm2 = "file://fby2_defconfig.append"
SRC_URI:append:mf-tpm2 = " file://fby2_vboot_defconfig.append"

do_configure:prepend:mf-tpm2() {
    cp -v ${WORKDIR}/fby2_vboot_defconfig.append ${WORKDIR}/fby2_defconfig.append
}

do_copyfile () {
  cp -v ${WORKDIR}/fby2_ext.h ${S}/include/configs/
}
addtask copyfile after do_patch before do_configure
