# Copyright 2017-present Facebook. All Rights Reserved.

SUMMARY = "BIOS Utility"
DESCRIPTION = "A BIOS Util for BIOS Related IPMI Command Setting"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://bios-head.py;beginline=1;endline=1;md5=28b35612b145c34c676817ce69fbfeb6"

LOCAL_URI = " \
    file://bios-head.py \
    file://bios_board.py \
    file://bios_boot_order.py \
    file://bios_ipmi_util.py \
    file://bios_postcode.py \
    file://bios_plat_info.py \
    file://bios_pcie_port_config.py \
    file://bios_default_support.json \
    file://bios_tpm_physical_presence.py \
    file://bios_force_clear_cmos.py \
    file://tests \
    "

inherit ptest

DEPENDS += "libpal"
DEPENDS += "update-rc.d-native"

BIOS_UTIL_CONFIG = "bios_default_support.json"

BIOS_UTIL_INIT_FILE = ""

binfiles = "bios-head.py bios_board.py bios_force_clear_cmos.py bios_boot_order.py bios_ipmi_util.py bios_postcode.py bios_plat_info.py bios_pcie_port_config.py bios_tpm_physical_presence.py"

pkgdir = "bios-util"

do_compile_ptest() {
  cat << EOF > ${WORKDIR}/run-ptest
#!/bin/sh
set -e
cd /usr/lib/bios-util/ptest
PYTHONPATH=/usr/local/fbpackages/bios-util python3 ./bios_tester.py
EOF
}

do_install_ptest() {
  install -d ${D}${libdir}/bios-util
  install -d ${D}${libdir}/bios-util/ptest
  cp -r ${S}/tests/* ${D}${libdir}/bios-util/ptest/
}

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  install -d $dst
  localbindir="${D}/usr/local/bin"
  install -d ${localbindir}
  install -d ${D}${sysconfdir}
  install -d ${D}${sysconfdir}/init.d

  for f in ${BIOS_UTIL_CONFIG}; do
    install -m 644 $f ${dst}/$f
  done
  if [ ! -z ${BIOS_UTIL_INIT_FILE} ]; then
    install -m 755 ${BIOS_UTIL_INIT_FILE} ${D}${sysconfdir}/init.d/
    update-rc.d -r ${D} ${BIOS_UTIL_INIT_FILE} start 91 5 .
  fi
  for f in ${binfiles}; do
    install -m 755 $f ${dst}/$f
  done

  ln -s ../fbpackages/${pkgdir}/bios-head.py ${localbindir}/bios-util
  cp ${dst}/bios_default_support.json ${dst}/bios_support.json

}

RDEPENDS:${PN} += "libpal"

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} += "${FBPACKAGEDIR}/bios-util ${prefix}/local/bin ${sysconfdir} "

RDEPENDS:${PN} += "python3-core"
