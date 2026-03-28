# Copyright 2022-present Facebook. All Rights Reserved.
do_install:append() {
  install -d ${D}${includedir}
  install -d ${D}${includedir}/linux
  install -m 0644 ${S}/include/linux/amd-apml.h ${D}${includedir}/linux/amd-apml.h
}
