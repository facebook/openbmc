#
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

FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += " \
    file://plat/meson.build \
    file://pal-ipmi.c \
    file://pal-ipmi.h \
    file://pal-led.c \
    file://pal-led.h \
    file://pal-power.c \
    file://pal-power.h \
    file://pal-sensors.c \
    file://pal-sensors.h \
    file://pal-debugcard.c \
    file://pal-debugcard.h \
    file://pal.c \
    file://pal.h \
    "

DEPENDS += " \
    libbic \
    libgpio-ctrl \
    liblog \
    libmisc-utils \
    libobmc-i2c \
    libsensor-correction \
    libwedge-eeprom \
    "

# These shouldn't be needed but are because we aren't properly versioning the
# shared libraries contained in these recipes.
RDEPENDS_${PN} += " \
    libbic \
    libgpio-ctrl \
    liblog \
    libmisc-utils \
    libobmc-i2c \
    libsensor-correction \
    libwedge-eeprom \
    "
