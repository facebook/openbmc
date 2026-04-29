inherit qemuboot

ZEPHYR_IMAGE_LINK_NAME ?= "${PN}-${MACHINE}"

KERNEL_IMAGETYPE = "${ZEPHYR_IMAGE_LINK_NAME}.elf"
QB_DEFAULT_FSTYPE = "elf"
QB_NETWORK_DEVICE = "none"
QB_NET = "none"
QB_ROOTFS = "none"

IMAGE_LINK_NAME = "${PN}-image-${MACHINE}"

# Create a link with "-image-" in the name just to keep runqemu happy
QEMU_IMAGE_LINK = "${DEPLOY_DIR_IMAGE}/${PN}-image-${MACHINE}.elf"

CLEANFUNCS += "bootconf_clean"

python bootconf_clean() {
    import glob
    files = glob.glob(d.getVar('IMGDEPLOYDIR', True)+'/'+ d.getVar('PN', True) + '*.qemuboot.conf')
    for f in files:
        os.remove(f)

    qemuimage_link = d.getVar('QEMU_IMAGE_LINK', True)
    if os.path.lexists(qemuimage_link):
        os.remove(qemuimage_link)
}

python do_bootconf_write() {
    bb.build.exec_func("do_write_qemuboot_conf", d)

    qemuimage = "%s/%s.elf" % (d.getVar('DEPLOY_DIR_IMAGE', True), d.getVar('ZEPHYR_IMAGE_LINK_NAME', True))
    qemuimage_link = d.getVar('QEMU_IMAGE_LINK', True)
    if os.path.lexists(qemuimage_link):
        os.remove(qemuimage_link)
    os.symlink(os.path.basename(qemuimage), qemuimage_link)
}

addtask do_bootconf_write before do_build after do_deploy

# The runqemu script requires the native sysroot populated for the
# qemu-helper-native recipes. Usually, this is pulled in by a do_image
# dependency (see baremetal-helloworld_git, for example), but in this case,
# there is no such task, so we hook in the dependency to do_bootconf_write.
# This also ensures that builds from sstate will also have this requirement
# satisfied.
python () {
    def extraimage_getdepends(task):
        deps = ""
        for dep in (d.getVar('EXTRA_IMAGEDEPENDS') or "").split():
        # Make sure we only add it for qemu
            if 'qemu-helper-native' in dep:
                deps += " qemu-helper-native:%s" % (task)
        return deps
    d.appendVarFlag('do_bootconf_write', 'depends', extraimage_getdepends('do_addto_recipe_sysroot'))
    d.appendVarFlag('do_bootconf_write', 'depends', extraimage_getdepends('do_populate_sysroot'))
}
