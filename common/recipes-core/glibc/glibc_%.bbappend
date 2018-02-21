FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

# The patch is for bug: 20116 which is fixed in glibc version 2.25.
# Thus, bring this in only for krogoth or fido, but leave it out
# for Rocko which packages glibc 2.26.
def glibc_patches(d):
    distro = d.getVar('DISTRO_CODENAME', True)
    if distro == 'fido' or distro == 'krogoth':
        return "file://Bug-20116-Fix-use-after-free-in-pthread_create.patch \
               "
    return ""

SRC_URI += '${@glibc_patches(d)}'
