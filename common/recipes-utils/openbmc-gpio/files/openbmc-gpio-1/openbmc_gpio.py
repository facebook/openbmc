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
import subprocess

_GPIO_SHADOW_DIR = '/tmp/gpionames'
_GPIO_CHIP_ASPEED = 'aspeed-gpio'


def _run_gpiocli(gpio_cmd,
                 chip=None, name=None, offset=None, shadow=None,
                 gpio_args=None):
    """Run gpiocli command with supplied options/arguments.
    """
    shell_cmd = "/usr/local/bin/gpiocli"
    if shadow:
        shell_cmd += ' --shadow %s' % shadow
    if chip:
        shell_cmd += ' --chip %s' % chip
    if name:
        shell_cmd += ' --pin-name %s' % name.upper()
    if offset:
        shell_cmd += ' --pin-offset %s' % str(offset)
    shell_cmd += ' %s' % gpio_cmd
    if gpio_args:
        shell_cmd += ' %s' % str(gpio_args)

    proc = subprocess.Popen(shell_cmd,
                            shell=True,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    out, err = proc.communicate()
    if len(err) != 0:
        raise Exception(err)
    return out.decode("utf-8")


def gpio_shadow_init(shadow_dir=None):
    """Set up shadow root directory if it doesn't exist.
    """
    global _GPIO_SHADOW_DIR

    if os.path.exists(_GPIO_SHADOW_DIR):
        shutil.rmtree(_GPIO_SHADOW_DIR)

    if shadow_dir:
        _GPIO_SHADOW_DIR = shadow_dir
    if not os.path.exists(_GPIO_SHADOW_DIR):
        os.makedirs(_GPIO_SHADOW_DIR)


def gpio_export(name, shadow, chip=_GPIO_CHIP_ASPEED):
    """Export control of the gpio pin to user space.
    """
    if shadow is None:
        shadow = '%s_%s' %  (chip, name)
    shadowdir = os.path.join(_GPIO_SHADOW_DIR, shadow)
    if os.path.exists(shadowdir):
        raise Exception('Shadow "%s" exists already' % shadowdir)

    print("exporting gpio (%s, %s), shadow=%s" % (chip, name, shadow))
    _run_gpiocli('export', name=name, chip=chip, shadow=shadow)


def _shadow_exists(name):
    """Check if the shadow file exists.
    """
    path = os.path.join(_GPIO_SHADOW_DIR, name)
    if os.path.islink(path):
        return True
    else:
        return False

def gpio_name_to_offset(name, chip=_GPIO_CHIP_ASPEED):
    """Map gpio pin name to offset within the gpio chip.
    """
    out = _run_gpiocli('map-name-to-offset', name=name, chip=chip)

    # "gpiocli" command output format: "<chip>,<name>, offset=<offset>"
    last_line = out.splitlines()[-1]
    offset = last_line.split('=')[1]
    return int(offset.strip())


def gpio_get_direction(name, chip=_GPIO_CHIP_ASPEED):
    """Get direction of the given gpio pin. The <name> argument could
       be pin name (such as GPIOA0) or shadow name.
    """
    if _shadow_exists(name):
        out = _run_gpiocli('get-direction', shadow=name)
    else:
        out = _run_gpiocli('get-direction', chip=chip, name=name)

    # "gpiocli" command output format: "direction=in/out"
    last_line = out.splitlines()[-1];
    direction = last_line.split('=')[1]
    return direction.strip()


def gpio_set_direction(name, direction, chip=_GPIO_CHIP_ASPEED):
    """Update direction of the given gpio pin. The <name> argument could
       be pin name (such as GPIOA0) or shadow name.
    """
    if _shadow_exists(name):
        _run_gpiocli('set-direction', gpio_args=direction, shadow=name)
    else:
        _run_gpiocli('set-direction', gpio_args=direction,
                     chip=chip, name=name)


def gpio_get_value(name, chip=_GPIO_CHIP_ASPEED, change_direction=True):
    """Get the current value of the given gpio pin. The <name> argument
       could be pin name (such as GPIOA0) or shadow name.
       The pin direction will be explicitly updated to <in> unless callers
       say no (for example, callers already set direction to <in>).
    """
    if change_direction:
        gpio_set_direction(name, 'in')

    if _shadow_exists(name):
        out = _run_gpiocli('get-value', shadow=name)
    else:
        out = _run_gpiocli('get-value', chip=chip, name=name)

    # "gpiocli" command output format: "value=0/1"
    last_line = out.splitlines()[-1]
    value = last_line.split('=')[1]
    return int(value.strip())


def gpio_set_value(name, value, chip=_GPIO_CHIP_ASPEED, change_direction=True):
    """Update the value of the given gpio pin. The <name> argument could
       be pin name (such as GPIOA0) or shadow name.
       The pin direction will be explicitly updated to <out> unless callers
       say no (for example, callers already set direction to <out>).
    """
    if change_direction:
        gpio_set_direction(name, 'out')

    if _shadow_exists(name):
        _run_gpiocli('set-value', gpio_args=str(value), shadow=name)
    else:
        _run_gpiocli('set-value', gpio_args=str(value),
                     chip=chip, name=name)


def gpio_info(name, chip=_GPIO_CHIP_ASPEED):
    res = {}

    if _shadow_exists(name):
        res['shadow'] = name
    else:
        res['shadow'] = None

    res['direction'] = gpio_get_direction(name, chip=chip)
    if res['direction'] == 'out':
       res['value'] = gpio_get_value(name, chip=chip)
    else:
       res['value'] = None

    return res
