inherit python3native

do_unit_test() {
  cd ${WORKDIR} && python3 -m unittest discover
}
addtask unit_test after do_compile before do_install
