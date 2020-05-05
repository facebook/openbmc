# Copyright 2018-present Facebook. All Rights Reserved.
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

do_install_append() {
    # We don't want to use the bind version of nslookup because
    # it has some issues using the search section of resolv.conf
    # when resolving non-fqdns.
    # Since busybox already installs nslookup (which doesn't have
    # the resolv.conf issue), delete the bind version of nslookup
    # here.

    nslookup_path="${D}${bindir}/nslookup"

    # some distros (e.g. rocko) already remove nslookup by default
    # remove nslookup bin only if it already exists.
    if [[ -e "$nslookup_path" ]]; then
        rm "$nslookup_path"
    fi
}
