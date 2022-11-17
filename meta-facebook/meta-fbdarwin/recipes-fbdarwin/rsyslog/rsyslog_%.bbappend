#
# Copyright 2022-present Facebook. All Rights Reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://rsyslog.service \
"

do_work_systemd() {
  install -m 644 ${WORKDIR}/rsyslog.service ${D}${systemd_unitdir}/system
}

do_install:append() {
  if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
    do_work_systemd
  fi
}
