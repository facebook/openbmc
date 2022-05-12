# Copyright 2014-present Facebook. All Rights Reserved.
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

SUMMARY = "Python Bottle IPC Evaluation"
DESCRIPTION = "Evaluate the impact of using Python Bottle as the ipc on CPU, memory, and latency."
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://src/ipc.py;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"

LOCAL_URI = " \
    file://src \
    "

do_install() {
  dst="${D}/usr/bin"
  install -d $dst
  install -m 755 src/service.py ${dst}/service.py
  install -m 755 src/ipc.py ${dst}/ipc.py
  install -m 755 src/caller-cputest.py ${dst}/caller-cputest.py
  install -m 755 src/caller-latencytest.py ${dst}/caller-latencytest.py
  install -m 755 src/caller-memorytest.py ${dst}/caller-memorytest.py
  install -m 755 src/service-memorytest.py ${dst}/service-memorytest.py
  install -m 755 src/ipc-memorytest.py ${dst}/ipc-memorytest.py
  install -m 755 src/memorytest.sh ${dst}/memorytest.sh
}
