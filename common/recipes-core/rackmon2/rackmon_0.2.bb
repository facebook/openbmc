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
inherit meson pkgconfig
inherit ptest-meson
inherit python3native
inherit python3-dir

SUMMARY = "Rackmon Functionality"
DESCRIPTION = "Rackmon Functionality"
SECTION = "base"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

DEPENDS:append = " update-rc.d-native"

DEPENDS += "glog libmisc-utils nlohmann-json cli11 python3-jsonschema-native"
RDEPENDS:${PN} = "glog libmisc-utils python3-core bash"

def get_profile_flag(d):
  prof_enabled = d.getVar("RACKMON_PROFILING", False)
  if prof_enabled:
    return "true"
  return "false"
EXTRA_OEMESON += "-Dprofiling=${@get_profile_flag(d)}"

def get_orv3_support(d):
  mc = d.getVar("MACHINE", "")
  if mc == "wedge100":
    return "false"
  return "true"
EXTRA_OEMESON += "-Dorv3=${@get_orv3_support(d)}"

LOCAL_URI = " \
    file://meson.build \
    file://meson_options.txt \
    file://rackmond.service \
    file://run-rackmond.sh \
    file://setup-rackmond.sh \
    file://Log.h \
    file://Device.cpp \
    file://Device.h \
    file://ModbusError.h \
    file://ModbusCmds.cpp \
    file://ModbusCmds.h \
    file://Modbus.cpp \
    file://Modbus.h \
    file://Msg.cpp \
    file://Msg.h \
    file://UARTDevice.cpp \
    file://UARTDevice.h \
    file://Register.cpp \
    file://Register.h \
    file://ModbusDevice.cpp \
    file://ModbusDevice.h \
    file://Rackmon.cpp \
    file://Rackmon.h \
    file://PollThread.h \
    file://UnixSock.cpp \
    file://UnixSock.h \
    file://RackmonSvcUnix.cpp \
    file://RackmonCliUnix.cpp \
    "
# Configuration files
LOCAL_URI += " \
    file://configs/interface/aspeed_uart.conf \
    file://configs/interface/usb_ft232.conf \
    file://configs/interface/usb_ft4232.conf \
    file://configs/register_map/orv2_psu.json \
    file://configs/register_map/orv3_psu.json \
    file://configs/register_map/orv3_bbu.json \
    file://configs/register_map/orv3_rpu.json \
    "

# Schemas
LOCAL_URI += " \
    file://schemas/RegisterMapConfigSchema.json \
    file://schemas/InterfaceConfigSchema.json \
    file://schemas/interface/interface.json \
    file://schemas/registermap/float_constraints.json \
    file://schemas/registermap/flags_constraints.json \
    file://schemas/registermap/integer_constraints.json \
    file://schemas/registermap/register.json \
    file://schemas/registermap/special_handlers.json \
    file://schemas/registermap/baud_config.json \
    "
#scripts
LOCAL_URI += " \
    file://scripts/schema_validator.py \
    file://scripts/pyrmd.py \
    file://scripts/rackmon-stress.py \
    "

# Test sources
LOCAL_URI += " \
    file://tests/Main.cpp \
    file://tests/MsgTest.cpp \
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
    file://tests/UnixSockTest.cpp \
    file://tests/TempDir.h \
    file://tests/test_pyrmd.py \
    "

install_wrapper() {
  echo "#!/bin/bash" > $2
  echo "set -e" >> $2
  echo $1 >> $2
  chmod 755 $2
}

install_systemd() {
    install -d ${D}${systemd_system_unitdir}

    install -m 0644 ${S}/rackmond.service ${D}${systemd_system_unitdir}
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
    bin="${D}/usr/local/bin"
    install_wrapper "/usr/local/bin/rackmoncli legacy_list" ${bin}/rackmonstatus
    install_wrapper "/usr/local/bin/rackmoncli data --format raw --json" ${bin}/rackmondata
    install_wrapper "/usr/local/bin/rackmoncli data --format value" ${bin}/rackmoninfo
    install_wrapper "/usr/local/bin/rackmoncli \$@" ${bin}/rackmonctl
    ln -snf ../bin/rackmoncli ${bin}/modbuscmd

    install -d ${D}${PYTHON_SITEPACKAGES_DIR}
    install -m 644 ${S}/scripts/pyrmd.py ${D}${PYTHON_SITEPACKAGES_DIR}/
}


FILES:${PN} = "${prefix}/local/bin ${sysconfdir} "

FILES:${PN} += "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', '${systemd_system_unitdir}', '', d)}"
FILES:${PN} += "${PYTHON_SITEPACKAGES_DIR}/pyrmd.py"

SYSTEMD_SERVICE:${PN} = "rackmond.service"
