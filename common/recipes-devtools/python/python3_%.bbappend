# Since we are using only python3, let's provide /usr/bin/python as a symlink to python3
do_install_append() {
  cd ${D}/${bindir}
  ln -s python3 python
}

FILES_${PN} += "${bindir}/python*"
