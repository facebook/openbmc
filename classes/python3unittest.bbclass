inherit python3native

do_unit_test() {
}
addtask unit_test after do_compile before do_install

do_unit_test_append_class-native() {
  cd ${WORKDIR} && python3 -m unittest discover
}
