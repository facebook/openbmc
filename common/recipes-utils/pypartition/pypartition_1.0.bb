# Copyright 2017-present Facebook. All Rights Reserved.
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

inherit python3unittest distutils3

SUMMARY = "Check U-Boot partitions"
DESCRIPTION = "Read, verify, and potentially modify U-Boot (firmware \
environment, legacy) partitions within image files or Memory Technology \
Devices."
SECTION = "base"
PR = "r3"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://partition.py;beginline=3;endline=16;\
md5=0b1ee7d6f844d472fa306b2fee2167e0"

SRC_URI = "file://check_image.py \
           file://improve_system.py \
           file://partition.py \
           file://setup.py \
           file://system.py \
           file://test_improve_system.py \
           file://test_partition.py \
           file://test_system.py \
           file://test_virtualcat.py \
           file://virtualcat.py \
           file://bmc_pusher"

S = "${WORKDIR}"

do_lint() {
  flake8 *.py
  mypy *.py
  shellcheck bmc_pusher
}
addtask do_lint after do_build
