# QEMU target list functions for qemu-master recipes
# This file must be included BEFORE qemu-master-native_git.inc to ensure
# functions are available before inherit native is processed.
#
# These functions are copied from poky's qemu-targets.inc to avoid issues
# with inherit native and function resolution on older Yocto branches
# that don't support inherit_defer.

def get_qemu_target_list(d):
    import bb
    archs = d.getVar('QEMU_TARGETS').split()
    tos = d.getVar('HOST_OS')
    tarch = d.getVar('HOST_ARCH')
    if "64" not in tarch:
        for arch in archs.copy():
            if "64" in arch:
                archs.remove(arch)
    softmmuonly = ""
    for arch in ['ppcemb', 'lm32']:
        if arch in archs:
            softmmuonly += arch + "-softmmu,"
            archs.remove(arch)
    linuxuseronly = ""
    for arch in ['armeb', 'alpha', 'ppc64abi32', 'ppc64le', 'sparc32plus', 'aarch64_be']:
        if arch in archs:
            linuxuseronly += arch + "-linux-user,"
            archs.remove(arch)
    if 'linux' not in tos:
        return softmmuonly + ''.join([arch + "-softmmu" + "," for arch in archs]).rstrip(',')
    return softmmuonly + linuxuseronly + ''.join([arch + "-linux-user" + "," + arch + "-softmmu" + "," for arch in archs]).rstrip(',')

def get_qemu_usermode_target_list(d):
    return ",".join(filter(lambda i: "-linux-user" in i, get_qemu_target_list(d).split(',')))

def get_qemu_system_target_list(d):
    return ",".join(filter(lambda i: "-linux-user" not in i, get_qemu_target_list(d).split(',')))
