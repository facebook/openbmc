FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
            file://busybox.cfg \
            file://setup_crond.sh \
           "

# The patch is for bug discovered on rocko which busybox version is 1.24.1.
# The patch has been included in newer version than 1.24.1.
def busybox_patches(d):
  pv = d.getVar('PV', True)
  if pv == '1.24.1':
    return "file://fix-uninitialized-memory-when-displaying-IPv6.patch \
           "
  return ""

SRC_URI += '${@busybox_patches(d)}'

DEPENDS_append = " update-rc.d-native"

do_install_append() {
  install -d ${D}${sysconfdir}/init.d
  install -m 755 ../setup_crond.sh ${D}${sysconfdir}/init.d/setup_crond.sh
  update-rc.d -r ${D} setup_crond.sh start 97 5 .
}
