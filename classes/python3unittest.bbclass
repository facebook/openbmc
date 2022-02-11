inherit python3native
RDEPENDS:${PN}-ptest += "python3-coverage"

do_unit_test() {
}
addtask unit_test after do_compile before do_install

do_unit_test:append:class-native() {
  cd ${WORKDIR} && python3 -m unittest discover
}
