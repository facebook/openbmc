IMAGE_CLASSES += "qemuboot"

def qemu_helper_for_distro(d):
    distro = d.getVar('DISTRO_CODENAME')

    if distro in [ 'master' ]:
        return "qemu-helper-native:do_addto_recipe_sysroot"

    return ""

EXTRA_IMAGEDEPENDS:append = " ${@qemu_helper_for_distro(d)}"

QB_SYSTEM_NAME = "qemu-system-arm"
QB_MACHINE = "-machine ${MACHINE}-bmc"
QB_MEM = "2G"
QB_RNG = ""
QB_ROOTFS = "none"
QB_OPT_APPEND = \
    "-drive file=@DEPLOY_DIR_IMAGE@/flash-${MACHINE},format=raw,if=mtd,snapshot=on \
     -drive file=@DEPLOY_DIR_IMAGE@/flash-${MACHINE},format=raw,if=mtd,snapshot=on"
QB_NETWORK_DEVICE = " -net nic,model=ftgmac100,netdev=net0 "
QB_DEFAULT_KERNEL = "none"
QB_DEFAULT_FSTYPE = "none"
QB_SERIAL_OPT = " -serial mon:stdio -serial null "
QB_GRAPHICS = ""
QB_SLIRP_OPT = " -netdev user,id=net0,mfr-id=0x8119,oob-eth-addr=aa:bb:cc:dd:ee:ff,hostfwd=::2222-:22 "
