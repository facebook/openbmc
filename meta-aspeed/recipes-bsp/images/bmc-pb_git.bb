# Copyright 2026-present Facebook. All Rights Reserved.
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

SUMMARY = "ASPEED BMC Prebuilt Binaries for AST27xx bring-up"
HOMEPAGE = "https://github.com/AspeedTech-BMC/bmc-pb"
PACKAGE_ARCH = "${MACHINE_ARCH}"

require bmc-pb.inc

PREBUILT_DIR ?= "ast2700a2"
PREBUILT_DIR:ast2700-a1 ?= "ast2700a1"
CALIPTRA_FW_BINARY ?= "caliptra-fw.bin"
SSMCU_ROM_BINARY ?= ""
SSMCU_RUNTIME_BINARY ?= ""
BOOTMCU_ROM_BINARY ?= ""

do_patch[noexec] = "1"
do_configure[noexec] = "1"
do_compile[noexec] = "1"
do_install[noexec] = "1"

inherit deploy

do_deploy () {
  install -d ${DEPLOYDIR}

  install -m 644 ${S}/${PREBUILT_DIR}/${CALIPTRA_FW_BINARY} ${DEPLOYDIR}
  install -m 644 ${S}/${PREBUILT_DIR}/ddr4_*.bin ${DEPLOYDIR}
  install -m 644 ${S}/${PREBUILT_DIR}/ddr5_*.bin ${DEPLOYDIR}
  install -m 644 ${S}/${PREBUILT_DIR}/dp_*.bin ${DEPLOYDIR}
  install -m 644 ${S}/${PREBUILT_DIR}/uefi_*.bin ${DEPLOYDIR}
  if [ -n "${SSMCU_ROM_BINARY}" ]; then
    install -m 644 ${S}/${PREBUILT_DIR}/${SSMCU_ROM_BINARY} ${DEPLOYDIR}
  fi
  if [ -n "${SSMCU_RUNTIME_BINARY}" ]; then
    install -m 644 ${S}/${PREBUILT_DIR}/${SSMCU_RUNTIME_BINARY} ${DEPLOYDIR}
  fi
  if [ -n "${BOOTMCU_ROM_BINARY}" ]; then
    install -m 644 ${S}/${PREBUILT_DIR}/${BOOTMCU_ROM_BINARY} ${DEPLOYDIR}
  fi
}

addtask deploy before do_build after do_compile
