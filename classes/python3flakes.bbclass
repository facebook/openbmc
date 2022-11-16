inherit python3native
def flakes_add_if_avail(d):
  distro = d.getVar('DISTRO_CODENAME', True)
  if distro in [ 'rocko' ]:
    return ""
  else:
    return "python3-flake8"

RDEPENDS:${PN}-ptest += "${@flakes_add_if_avail(d)}"
do_flake8() {
}
addtask do_flake8 after do_unit_test before do_install

do_flake8:append:class-native() {
  cd ${WORKDIR} && flake8 --select=F,C90  *.py
}
