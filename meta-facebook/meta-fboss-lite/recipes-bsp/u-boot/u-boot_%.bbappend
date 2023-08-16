FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://facebook-fboss-lite_defconfig.append \
           "

do_merge_uboot_configs() {
    base_config="${S}/configs/${UBOOT_CONFIG_BASE}"
    def_config="${WORKDIR}/facebook-fboss-lite_defconfig.append"

    if [ -e "$base_config" -a -e "$def_config" ]; then
        bbnote "Merging ${UBOOT_CONFIG_BASE} with $def_config"
        KCONFIG_CONFIG=${S}/configs/${UBOOT_CONFIG_BASE} \
            ${S}/scripts/kconfig/merge_config.sh -m "$base_config" "$def_config"
    else
        bbfatal "FBOSS-lite UBOOT_CONFIG_BASE or facebook-fboss-lite_defconfig.append not found!"
    fi
}

addtask do_merge_uboot_configs after do_patch before do_configure