FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://fby2_ext.h \
           "

SRC_URI += '${@bb.utils.contains("MACHINE_FEATURES", "tpm2", "file://fby2_vboot_defconfig.append ", "file://fby2_defconfig.append ", d)}'

do_configure_prepend() {
  if ! echo ${MACHINE_FEATURES} | awk "/tpm2/ {exit 1}"; then
    cp -v ${WORKDIR}/fby2_vboot_defconfig.append ${WORKDIR}/fby2_defconfig.append
  fi
}

do_copyfile () {
  cp -v ${WORKDIR}/fby2_ext.h ${S}/include/configs/
}
addtask copyfile after do_patch before do_configure
