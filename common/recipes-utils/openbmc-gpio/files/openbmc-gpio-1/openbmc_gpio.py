#!/usr/bin/env python3
# Copyright 2015-present Facebook. All rights reserved.
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
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import logging
import os
import string
import shutil


_gpio_shadow = '/tmp/gpionames'


def setup_shadow(shadow=None):
    global _gpio_shadow
    if os.path.exists(_gpio_shadow):
        shutil.rmtree(_gpio_shadow)
    if shadow is not None:
        _gpio_shadow = shadow
    if not os.path.exists(_gpio_shadow):
        os.makedirs(_gpio_shadow)


def gpio_name2value(name):
    name = str(name).lower()
    if name.startswith('gpio'):
        name = name[4:]
    try:
        return int(name)
    except:
        # it is not just value, try treat it as name like 'A0'
        pass
    val = 0
    try:
        if len(name) != 2 and len(name) != 3:
            raise
        for idx in range(0, len(name)):
            ch = name[idx]
            if ch in string.ascii_lowercase:
                # letter cannot be the last character
                if idx == len(name) - 1:
                    raise
                tmp = ord(ch) - ord('a') + 1
                val = val * 26 + tmp
            elif ch in string.digits:
                # digit must be the last character
                if idx != len(name) - 1:
                    raise
                # the digit must be 0-7
                tmp = ord(ch) - ord('0')
                if tmp > 7:
                    raise
                # 'A4' is value 4
                if val > 0:
                    val -= 1
                val = val * 8 + tmp
            else:
                raise
    except:
        logging.exception('Invalid GPIO name "%s"' % name)
    return val


def gpio_dir(name, check_shadow=True):
    GPIODIR_FMT = '/sys/class/gpio/gpio{gpio}'
    if check_shadow:
        shadowdir = os.path.join(_gpio_shadow, name)
        if os.path.isdir(shadowdir):
            return shadowdir
    val = gpio_name2value(name)
    return GPIODIR_FMT.format(gpio=val)


def gpio_get_shadow(name):
    path = gpio_dir(name, check_shadow=False)
    for child in os.listdir(_gpio_shadow):
        try:
            child = os.path.join(_gpio_shadow, child)
            if os.readlink(child) == path:
                return child
        except:
            pass
    return None


def gpio_export(name, shadow=None):
    GPIOEXPORT = '/sys/class/gpio/export'
    if shadow is not None or shadow != '':
        shadowdir = os.path.join(_gpio_shadow, shadow)
        if os.path.exists(shadowdir):
            raise Exception('Shadow "%s" exists already' % shadowdir)
        old_shadow = gpio_get_shadow(name)
        if old_shadow is not None:
            raise Exception('Shadow "%s" already exists for %s'
                            % (old_shadow, name))

    val = gpio_name2value(name)
    try:
        with open(GPIOEXPORT, 'w') as f:
            f.write('%d\n' % val)
    except:
        # in case the GPIO has been exported already
        pass
    if shadow is not None:
        gpiodir = gpio_dir(val, check_shadow=False)
        os.symlink(gpiodir, shadowdir)


def gpio_get(name, change_direction=True):
    path = gpio_dir(name)
    if change_direction:
        with open(os.path.join(path, 'direction'), 'w') as f:
            f.write('in\n')
    with open(os.path.join(path, 'value'), 'r') as f:
        val = int(f.read().rstrip('\n'))
    return val


def gpio_set(name, value, change_direction=True):
    path = gpio_dir(name)
    if change_direction:
        with open(os.path.join(path, 'direction'), 'w') as df:
            df.write('%s\n' %("high" if value else "low"))
            df.close()


def gpio_info(name):
    res = {}
    # first check if name is shadow
    path = None
    shadow = os.path.join(_gpio_shadow, name)
    if os.path.exists(shadow):
        if not os.path.islink(shadow) or not os.path.isdir(shadow):
            raise Exception('Path "%s" is not a valid shadow path' % shadow)
        path = os.readlink(shadow)
    else:
        path = gpio_dir(name, check_shadow=False)
        shadow = gpio_get_shadow(name)
    res['path'] = path
    res['shadow'] = shadow
    if os.path.isdir(path):
        with open(os.path.join(path, 'direction'), 'r') as f:
            res['direction'] = f.read().rstrip('\n')
        with open(os.path.join(path, 'value'), 'r') as f:
            res['value'] = int(f.read().rstrip('\n'))
    else:
        res['direction'] = None
        res['value'] = None
    return res
