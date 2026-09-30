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

DEPENDS += "glog libmisc-utils nlohmann-json cli11 python3-jsonschema-native systemd"
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

S = "${UNPACKDIR}"
LOCAL_URI = " \
    file://meson.build \
    file://meson_options.txt \
    file://rackmond.service.in \
    file://Log.h \
    file://Device.cpp \
    file://Device.h \
    file://DeviceLocationFilter.h \
    file://DeviceLocationIterator.h \
    file://InterfaceScanner.cpp \
    file://InterfaceScanner.h \
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
    file://ModbusDeviceInventory.cpp \
    file://ModbusDeviceInventory.h \
    file://Rackmon.cpp \
    file://Rackmon.h \
    file://PollThread.h \
    file://UnixSock.cpp \
    file://UnixSock.h \
    file://RackmonSvcUnix.cpp \
    file://RackmonCliUnix.cpp \
    file://ModbusUtil.cpp \
    "
# Configuration files
LOCAL_URI += " \
    file://configs/interface/aspeed_uart.conf \
    file://configs/interface/usb_ft232.conf \
    file://configs/interface/usb_ft4232.conf \
    file://configs/interface/ventura.conf \
    file://configs/interface/ventura2.conf \
    file://configs/register_map/orv2_psu.json \
    file://configs/register_map/orv3_psu.json \
    file://configs/register_map/orv3_bbu.json \
    file://configs/register_map/orv3_rpu.json \
    file://configs/register_map/orv3_rpu2.json \
    file://configs/register_map/orv3_hpr_pmm_psu.json \
    file://configs/register_map/orv3_hpr_pmm_bbu.json \
    file://configs/register_map/orv3_hpr_pmm_cbu.json \
    file://configs/register_map/orv3_hpr_psu.json \
    file://configs/register_map/orv3_hpr_bbu.json \
    file://configs/register_map/orv3_hpr_cbu.json \
    file://configs/register_map/miniups.json \
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
    "
#scripts
LOCAL_URI += " \
    file://scripts/schema_validator.py \
    file://scripts/address_validator.py \
    file://scripts/pyrmd.py \
    file://scripts/rackmon-stress.py \
    file://scripts/table-rackmon-dump \
    file://scripts/table-rackmon-list \
    file://scripts/hpr-power-off.sh \
    "

# Test sources
LOCAL_URI += " \
    file://tests/Main.cpp \
    file://tests/MsgTest.cpp \
    file://tests/DeviceLocationIteratorTest.cpp \
    file://tests/DeviceTest.cpp \
    file://tests/ModbusCmdsTest.cpp \
    file://tests/ModbusTest.cpp \
    file://tests/RegisterDescriptorTest.cpp \
    file://tests/RegisterValueTest.cpp \
    file://tests/RegisterTest.cpp \
    file://tests/RegisterMapTest.cpp \
    file://tests/RegisterSpanTest.cpp \
    file://tests/ModbusDeviceTest.cpp \
    file://tests/PollThreadTest.cpp \
    file://tests/RackmonTest.cpp \
    file://tests/UnixSockTest.cpp \
    file://tests/TempDir.h \
    file://tests/TestUtils.h \
    file://tests/test_pyrmd.py \
    "

install_wrapper() {
  echo "#!/bin/bash" > $2
  echo "set -e" >> $2
  echo $1 >> $2
  chmod 755 $2
}

INTERFACE_CONFIG ?= "/usr/share/rackmon/interface/aspeed_uart.conf"
INTERFACE_CONFIG:ventura = "/usr/share/rackmon/interface/ventura.conf"
INTERFACE_CONFIG:ventura2 = "/usr/share/rackmon/interface/ventura2.conf"
INTERFACE_CONFIG:wedge400 = "\${INTERFACE_CONFIG_FILE}"

SERVICE_AFTER ?= ""
SERVICE_AFTER:openbmc-fb ?= "After=setup_i2c.service openbmc_gpio_setup.service"

SERVICE_WANT ?= ""
SERVICE_WANT:openbmc-fb ?= "Wants=openbmc_gpio_setup.service setup_board.service"

do_install:append() {
    sed -i -e "s:REPLACE_WITH_INTERFACE_CONFIG_PATH:${INTERFACE_CONFIG}:" ${UNPACKDIR}/rackmond.service.in
    sed -i -e "s:REPLACE_WITH_REGMAP_CONFIG_DIR_PATH:/usr/share/rackmon/registermap:" ${UNPACKDIR}/rackmond.service.in

    sed -i -e "s:REPLACE_WITH_SERVICE_AFTER:${SERVICE_AFTER}:" ${UNPACKDIR}/rackmond.service.in
    sed -i -e "s:REPLACE_WITH_SERVICE_WANT:${SERVICE_WANT}:" ${UNPACKDIR}/rackmond.service.in

    install -d ${D}${systemd_system_unitdir}

    install -m 0644 ${UNPACKDIR}/rackmond.service.in ${D}${systemd_system_unitdir}/rackmond.service

    bin="${D}/usr/local/bin"
    install_wrapper "/usr/local/bin/rackmoncli legacy_list" ${bin}/rackmonstatus
    install_wrapper "/usr/local/bin/rackmoncli data --format raw --json" ${bin}/rackmondata
    install_wrapper "/usr/local/bin/rackmoncli data --format value" ${bin}/rackmoninfo
    install_wrapper "/usr/local/bin/rackmoncli \$@" ${bin}/rackmonctl
    ln -snf ../bin/rackmoncli ${bin}/modbuscmd

    install -d ${D}${PYTHON_SITEPACKAGES_DIR}
    install -m 644 ${UNPACKDIR}/scripts/pyrmd.py ${D}${PYTHON_SITEPACKAGES_DIR}/

    install -m 755 ${UNPACKDIR}/scripts/table-rackmon-dump ${bin}/table-rackmon-dump
    install -m 755 ${UNPACKDIR}/scripts/table-rackmon-list ${bin}/table-rackmon-list
    install -m 755 ${UNPACKDIR}/scripts/hpr-power-off.sh ${bin}/hpr-power-off
}

FILES:${PN} = "${prefix}/local/bin ${sysconfdir} "
FILES:${PN} += "/usr/share/rackmon /usr/share/rackmon/interface /usr/share/rackmon/registermap "

FILES:${PN} += "${systemd_system_unitdir}"
FILES:${PN} += "${PYTHON_SITEPACKAGES_DIR}/pyrmd.py"

SYSTEMD_SERVICE:${PN} = "rackmond.service"
