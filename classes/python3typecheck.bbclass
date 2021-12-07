inherit python3native
def mypy_add_if_avail(d):
  distro = d.getVar('DISTRO_CODENAME', True)
  print(distro)
  if distro in ['dunfell']:
    return "python3-mypy"
  else:
    return ""

RDEPENDS:${PN}-ptest += "${@mypy_add_if_avail(d)}"

do_typecheck() {
}
addtask typecheck after do_unit_test before do_flake8

do_typecheck:append:class-native() {
  cd ${WORKDIR} && mypy *.py
}
