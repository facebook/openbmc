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
FILESEXTRAPATHS_prepend := "${THISDIR}/files/:"
SRC_URI  += " file://setup-ipmbd.sh \
              file://run-ipmbd_0.sh \ 
              file://run-ipmbd_1.sh \
              file://run-ipmbd_2.sh \
              file://run-ipmbd_3.sh \
            "
CFLAGS_prepend = " -DCONFIG_FBY3_POC "

do_install_append() {
  #remove the old setting
  rm -rf ${D}${sysconfdir}/sv/ipmbd_1
  rm -rf ${D}${sysconfdir}/sv/ipmbd_3
  rm -rf ${D}${sysconfdir}/sv/ipmbd_5
  rm -rf ${D}${sysconfdir}/sv/ipmbd_7
  rm -rf ${D}${sysconfdir}/sv/ipmbd_13
  rm -rf ${D}${sysconfdir}/ipmbd_1
  rm -rf ${D}${sysconfdir}/ipmbd_3
  rm -rf ${D}${sysconfdir}/ipmbd_5
  rm -rf ${D}${sysconfdir}/ipmbd_7
  rm -rf ${D}${sysconfdir}/ipmbd_13

  #install the new setting
  install -d ${D}${sysconfdir}/sv/ipmbd_0
  install -d ${D}${sysconfdir}/sv/ipmbd_1
  install -d ${D}${sysconfdir}/sv/ipmbd_2
  install -d ${D}${sysconfdir}/sv/ipmbd_3

  install -m 755 run-ipmbd_0.sh ${D}${sysconfdir}/sv/ipmbd_0/run
  install -m 755 run-ipmbd_1.sh ${D}${sysconfdir}/sv/ipmbd_1/run
  install -m 755 run-ipmbd_2.sh ${D}${sysconfdir}/sv/ipmbd_2/run
  install -m 755 run-ipmbd_3.sh ${D}${sysconfdir}/sv/ipmbd_3/run
}
