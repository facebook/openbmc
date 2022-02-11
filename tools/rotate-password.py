#!/usr/bin/env python3
#
# Copyright 2017-present Facebook. All Rights Reserved.
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

from crypt import crypt, mksalt
from sys import argv, stdin
from time import time

days_since_epoch = int(time() / 86400)
password_hash = crypt(stdin.readline().splitlines()[0], mksalt())
print(f'root:{password_hash}:{days_since_epoch}:0:99999:7:::')

with open(argv[1], 'r') as shadow:
    old_shadow_file_contents = shadow.readlines()

[print(line, end='') for line in old_shadow_file_contents[1:]]
