# Copyright 2026-present Facebook. All Rights Reserved.
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

# AST2700's firmware bundle occupies the first 4000 KiB of flash, placing
# the Facebook image metadata at 0x003e8000.  The image validator reads
# the legacy AST2600 metadata offset at 0x000f0000 by default, so it needs
# to be overriden.
CXXFLAGS:append:aristabmc = " -DFW_UTIL_IMAGE_META_OFFSET=0x003E8000"