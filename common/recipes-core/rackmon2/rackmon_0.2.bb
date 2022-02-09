# Copyright 2021-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
inherit systemd
inherit meson
inherit ptest-meson

SUMMARY = "Rackmon Functionality"
DESCRIPTION = "Rackmon Functionality"
SECTION = "base"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

DEPENDS:append = " update-rc.d-native"

DEPENDS += "liblog libmisc-utils nlohmann-json cli11"
RDEPENDS:${PN} = "liblog libmisc-utils python3-core bash"

def get_profile_flag(d):
  prof_enabled = d.getVar("RACKMON_PROFILING", False)
  if prof_enabled:
    return "true"
  return "false"
EXTRA_OEMESON += "-Dprofiling=${@get_profile_flag(d)}"


SRC_URI = "file://meson.build \
           file://meson_options.txt \
           file://rackmond.service \
           file://run-rackmond.sh \
           file://setup-rackmond.sh \
           file://Log.hpp \
           file://Device.cpp \
           file://Device.hpp \
           file://ModbusCmds.cpp \
           file://ModbusCmds.hpp \
           file://Modbus.cpp \
           file://Modbus.hpp \
           file://Msg.cpp \
           file://Msg.hpp \
           file://UARTDevice.cpp \
           file://UARTDevice.hpp \
           file://Register.cpp \
           file://Register.hpp \
           file://ModbusDevice.cpp \
           file://ModbusDevice.hpp \
           file://Rackmon.cpp \
           file://Rackmon.hpp \
           file://PollThread.hpp \
           file://RackmonSock.cpp \
           file://RackmonSvcUnix.hpp \
           file://RackmonSvcUnix.cpp \
           file://RackmonCliUnix.cpp \
          "
# Configuration files
SRC_URI += "file://rackmon.conf \
            file://rackmon.d/orv2_psu.json \
           "

# Test sources
SRC_URI += "file://tests/MsgTest.cpp \
            file://tests/DeviceTest.cpp \
            file://tests/ModbusCmdsTest.cpp \
            file://tests/ModbusTest.cpp \
            file://tests/RegisterDescriptorTest.cpp \
            file://tests/RegisterValueTest.cpp \
            file://tests/RegisterTest.cpp \
            file://tests/RegisterMapTest.cpp \
            file://tests/ModbusDeviceTest.cpp \
            file://tests/PollThreadTest.cpp \
            file://tests/RackmonTest.cpp \
            file://tests/TempDir.hpp \
           "

S = "${WORKDIR}"

install_wrapper() {
  echo "#!/bin/bash" > $2
  echo "set -e" >> $2
  echo $1 >> $2
  chmod 755 $2
}

install_systemd() {
    install -d ${D}${systemd_system_unitdir}

    install -m 0644 rackmond.service ${D}${systemd_system_unitdir}
}

install_sysv() {
    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d
    install -d ${D}${sysconfdir}/sv
    install -d ${D}${sysconfdir}/sv/rackmond
    install -m 755 ${S}/run-rackmond.sh ${D}${sysconfdir}/sv/rackmond/run
    install -m 755 ${S}/setup-rackmond.sh ${D}${sysconfdir}/init.d/rackmond
    update-rc.d -r ${D} rackmond start 95 2 3 4 5  .
}

do_install:append() {
    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        install_systemd
    else
        install_sysv
    fi
    install -d ${D}${sysconfdir}/rackmon.d
    install -m 644 ${S}/rackmon.conf ${D}${sysconfdir}/rackmon.conf
    for f in ${S}/rackmon.d/*; do
      install ${f} ${D}${sysconfdir}/rackmon.d/
    done
    bin="${D}/usr/local/bin"
    install_wrapper "/usr/local/bin/rackmoncli list" ${bin}/rackmonstatus
    install_wrapper "/usr/local/bin/rackmoncli data --json" ${bin}/rackmondata
    install_wrapper "/usr/local/bin/rackmoncli data --format" ${bin}/rackmoninfo
    install_wrapper "/usr/local/bin/rackmoncli raw \$@" ${bin}/modbuscmd
    install_wrapper "/usr/local/bin/rackmoncli \$@" ${bin}/rackmonctl
}


FILES:${PN} = "${prefix}/local/bin ${sysconfdir} "

FILES:${PN} += "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', '${systemd_system_unitdir}', '', d)}"

SYSTEMD_SERVICE:${PN} = "rackmond.service"
