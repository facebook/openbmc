# Since we are using only python3, let's provide /usr/bin/python as a symlink to python3
do_install_append() {
  ln -s /usr/bin/python3 ${D}/usr/bin/python
}

FILES_${PN} += "${bindir}/python3 ${bindir}/python"
