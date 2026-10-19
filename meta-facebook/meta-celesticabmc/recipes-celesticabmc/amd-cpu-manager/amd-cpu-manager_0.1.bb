# Copyright (c) Meta Platforms, Inc. and affiliates.
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

SUMMARY = "AMD CPU Manager: temperature threshold settings;"
DESCRIPTION = "When the `xyz.openbmc_project.State.Boot.Progress.ProgressStages.OSStart` state is captured, the application sets the CPU temperature threshold from the BMC."

LICENSE = "CLOSED"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:${THISDIR}:"

inherit meson
inherit pkgconfig
inherit systemd

def get_service(d):
    return "amd-cpu-manager.service"

SYSTEMD_SERVICE:${PN} = "${@get_service(d)}"

SRC_URI = "file://amd-cpu-manager"

PV = "0.1"
S = "${UNPACKDIR}/amd-cpu-manager"


DEPENDS += "\
    sdbusplus \
    apml \
    i2c-tools \
    phosphor-dbus-interfaces \
    phosphor-logging \
"

FILES:${PN} += "\
    ${systemd_unitdir}/system/${SYSTEMD_SERVICE:${PN}} \
"
