# Copyright 2020-present Facebook. All Rights Reserved.
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
#
#
# GrandCanyon 2.0 Project

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

LOCAL_URI += "\
    file://grandcanyon2.json \
    "

do_unpack:append () {
    bb.build.exec_func('do_replace_image_parts_json', d)
}

do_replace_image_parts_json() {
    if [ -f ${S}/grandcanyon2.json ]; then
        bbnote "replace common image_parts.json with grandcanyon2.json"
        mv -vf ${S}/image_parts.json ${S}/image_parts-generic.json
        mv -vf ${S}/grandcanyon2.json ${S}/image_parts.json
    else
        bbwarn "grandcanyon2.json not found in ${S}, skipping replacement"
    fi
}

CXXFLAGS:prepend = " -DCONFIG_GRANDCANYON2 "
