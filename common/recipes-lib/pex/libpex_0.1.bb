# Copyright 2022-present Facebook. All Rights Reserved.
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

SUMMARY = "PEX Library"
DESCRIPTION = "library for pex88000"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"

# The license GPL-2.0 was removed in Hardknott.                                                                                                                            
# Use GPL-2.0-or-later instead.                                                                                                                                            
def lic_file_name(d):                                                                                                                                                      
    distro = d.getVar('DISTRO_CODENAME', True)                                                                                                                             
    if distro in [ 'rocko', 'dunfell' ]:                                                                                                                                   
        return "GPL-2.0;md5=801f80980d171dd6425610833a22dbe6"                                                                                                              
                                                                                                                                                                           
    return "GPL-2.0-or-later;md5=fed54355545ffd980b814dab4a3b312c"                                                                                                         
LIC_FILES_CHKSUM = "\                                                                                                                                                      
    file://${COREBASE}/meta/files/common-licenses/${@lic_file_name(d)} \                                                                                                   
    "

LOCAL_URI = " \
    file://meson.build \
    file://pex88000.c \
    file://pex88000.h \
    "

DEPENDS += "libobmc-pmbus libobmc-i2c libmisc-utils "
RDEPENDS:${PN} += "libmisc-utils "

inherit meson pkgconfig
