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
#

SUMMARY = "Watchdog daemon"
DESCRIPTION = "Util for petting watchdog"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://watchdogd.sh;beginline=4;endline=16;md5=c5df8524e560f89f6fe75bb131d6e14d"

LOCAL_URI = " \
    file://watchdogd.sh \
    file://setup-watchdogd.sh \
    "

RDEPENDS:${PN} += " python3 bash"
DEPENDS:append = " update-rc.d-native"

# Why do rocko needs python3 and krogoth didn't?
#
# First, it's important to mention that when you run bitbake,you have several functions from .bbclass files which scanned your recipe and get the build going. 
# The watchdog daemon recipe is being scanned by the do_package_qa recipe task. This yocto package function call the package_qa_check_rdepends function which
# will check all the dependencies in this watchdogd_0.1.bb recipe. It expects to have python in the dependency list for python-core and skip checking /usr/bin/python
# if python is in the dependency list. Please read go to the codes that I mentioned at the end of this note for more details of what's going on.
# That's the case for both rocko and krogoth. For krogoth, if it doesn't find the python dependency it treats it as a warning and continue.
# Rocko is a lot stricter and treats it as an error. Unlike krogoth, rocko doesn't add the code to override it. 
#
# More details can be found here (check both rocko and krogoth).
# Files: yocto/rocko/poky/meta/classes/insane.bbclass 
# Function: search for package_qa_check_rdepends
#
# Read the codes and you will see where it is expecting python dependency to be part of the recipe file and why, and this will make more sense to you. 
#

do_install() {
  install -d ${D}${bindir}
  install -m 0755 watchdogd.sh ${D}${bindir}/watchdogd.sh

  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 setup-watchdogd.sh ${D}${sysconfdir}/init.d/setup-watchdogd.sh
  update-rc.d -r ${D} setup-watchdogd.sh start 95 2 3 4 5  .
}
FILES:${PN} = "${bindir}"
FILES:${PN} += "${sysconfdir}"
