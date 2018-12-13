inherit python3native

do_unit_test() {
  python3 -m unittest discover
}
addtask do_unit_test after do_build
