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

from soc_gpio import soc_get_register, soc_get_tolerance_reg

import openbmc_gpio
import logging
import phymemory
import os
import sys


class NotSmartEnoughException(Exception):
    '''There are few cases the code cannot make good decision on how to configure
    the registers automatically. In such cases, this exception is thrown.
    '''
    pass


class ConfigUnknownFunction(Exception):
    '''Unknown function to configure exception'''
    pass


class BitsEqual(object):
    def __init__(self, register, bits, value):
        self.register = register
        self.bits = bits
        self.value = value

    def __str__(self):
        return '%s[%s]==0x%x' \
            % (str(soc_get_register(self.register)), self.bits, self.value)

    def get_registers(self):
        return set([self.register])

    def check(self):
        return soc_get_register(self.register).bits_value(self.bits) \
            == self.value

    def satisfy(self, **kwargs):
        if BitsEqual.check(self):
            return
        reg = soc_get_register(self.register)
        value = self.value
        for bit in sorted(self.bits):
            if value & 0x1 == 0x1:
                reg.set_bit(bit, **kwargs)
            else:
                reg.clear_bit(bit, **kwargs)
            value >>= 1

    def unsatisfy(self, **kwargs):
        if not BitsEqual.check(self):
            return
        bit = self.bits[0]
        reg = soc_get_register(self.register)
        value = self.value
        for bit in sorted(self.bits):
            if value & 0x1 == 0x1:
                reg.clear_bit(bit, **kwargs)
            else:
                reg.set_bit(bit, **kwargs)


class BitsNotEqual(BitsEqual):
    def __str__(self):
        return '%s[%s]!=0x%x' \
            % (str(soc_get_register(self.register)), self.bits, self.value)

    def check(self):
        return not BitsEqual.check(self)

    def satisfy(self, **kwargs):
        BitsEqual.unsatisfy(self, **kwargs)

    def unsatisfy(self, **kwargs):
        BitsEqual.satisfy(self, **kwargs)


class AndOrBase(object):
    def __init__(self, left, right):
        self.left = left
        self.right = right

    def get_registers(self):
        return self.left.get_registers() | self.right.get_registers()

    def check(self):
        raise Exception('This method must be implemented in subclass')


class And(AndOrBase):
    def __str__(self):
        return 'AND(%s, %s)' % (str(self.left), str(self.right))

    def check(self):
        return self.left.check() and self.right.check()

    def satisfy(self, **kwargs):
        if self.check():
            return
        self.left.satisfy(**kwargs)
        self.right.satisfy(**kwargs)

    def unsatisfy(self, **kwargs):
        if not self.check():
            return
        raise NotSmartEnoughException('Not able to unsatisfy an AND condition')


class Or(AndOrBase):
    def __str__(self):
        return 'OR(%s, %s)' % (str(self.left), str(self.right))

    def check(self):
        return self.left.check() or self.right.check()

    def satisfy(self, **kwargs):
        if self.check():
            return
        raise NotSmartEnoughException('Not able to satisfy an OR condition')

    def unsatisfy(self, **kwargs):
        if not self.check():
            return
        self.left.unsatisfy(**kwargs)
        self.right.unsatisfy(**kwargs)


class Function(object):
    def __init__(self, name, condition=None):
        self.name = name
        self.condition = condition

    def __str__(self):
        return 'Function(\'%s\', %s)' % (self.name, str(self.condition))


class SocGPIOTable(object):
    def __init__(self, gpio_table):
        self.soc_gpio_table = gpio_table
        self.registers = set([])  # all HW registers used for GPIO control
        self.functions = {}

        self._parse_gpio_table()
        self._sync_from_hw()

    def _parse_gpio_table(self):
        # first get list of registers based on the SoC GPIO table
        for pin, funcs in self.soc_gpio_table.items():
            for func in funcs:
                assert func.name not in self.functions
                self.functions[func.name.split('/')[0]] = pin
                if func.condition is not None:
                    self.registers |= func.condition.get_registers()

    def _sync_from_hw(self):
        # for each register, create an object and read the value from HW
        for reg in self.registers:
            soc_get_register(reg).read(refresh=True)

    def write_to_hw(self):
        for reg in self.registers:
            soc_get_register(reg).write()

    def config_function(self, func_name, write_through=True):
        logging.debug('Configure function "%s"' % func_name)
        if func_name not in self.functions:
            # The function is not multi-function pin
            raise ConfigUnknownFunction('Unknown function "%s" ' % func_name)
        funcs = self.soc_gpio_table[self.functions[func_name]]
        for func in funcs:
            cond = func.condition
            if func.name.startswith(func_name):
                # this is the function we want to configure.
                # if the condition is None, we are good to go,
                # otherwiset, satisfy the condition
                if cond is not None:
                    cond.satisfy(write_through=write_through)
                break
            else:
                # this is not the funciton we want to configure.
                # have to make this condition unsatisfied, so that we can go
                # to the next function
                assert cond is not None
                cond.unsatisfy(write_through=write_through)

    def _get_one_pin(self, pin, refresh):
        if refresh:
            self._sync_from_hw()
        funcs = self.soc_gpio_table[pin]
        active_func = None
        all_funcs = []
        for func in funcs:
            cond = func.condition
            all_funcs.append('%s:%s' % (func.name, str(cond)))
            if active_func is None and (cond is None or cond.check()):
                active_func = func.name

        if active_func is None:
            logging.error('Pin "%s" has no function set up. '
                          'All possibile functions are %s.'
                          % (pin, ', '.join(all_funcs)))
            return ('', '')
        else:
            desc = '%s => %s, functions: %s' \
                % (pin, active_func, ', '.join(all_funcs))
            return (active_func, desc)

    def dump_pin(self, pin, out=sys.stdout, refresh=False):
        if pin not in self.soc_gpio_table:
            raise Exception('"%s" is not a valid pin' % pin)

        _, desc = self._get_one_pin(pin, refresh)
        out.write('%s\n' % desc)

    def dump_function(self, func_name, out=sys.stdout, refresh=False):
        if func_name not in self.functions:
            raise Exception('"%s" is not a valid function name' % func_name)
        pin = self.functions[func_name]
        self.dump_pin(pin, out=out, refresh=refresh)

    def dump_functions(self, out=sys.stdout, refresh=False):
        if refresh:
            self._sync_from_hw()

        for pin in self.soc_gpio_table:
            self.dump_pin(pin, out=out, refresh=False)

    def get_active_functions(self, refresh=False):
        if refresh:
            self._sync_from_hw()

        all = []
        for pin in self.soc_gpio_table:
            active, _ = self._get_one_pin(pin, False)
            all.append(active.split('/')[0])
        return all


GPIO_INPUT = 'input'
GPIO_OUT_HIGH = 'high'
GPIO_OUT_LOW = 'low'

class BoardGPIO(object):
    def __init__(self, gpio, shadow, value=GPIO_INPUT):
        self.gpio = gpio
        self.shadow = shadow
        self.value = value

def setup_board_tolerance_gpio(board_tolerance_gpio_table,
                               board_gpioOffsetDic, validate=True):
    gpio_configured = []
    regs = []
    for gpio in board_tolerance_gpio_table:
        if not (gpio.gpio).startswith('GPIO'):
            continue
        offset = openbmc_gpio.gpio_name_to_offset(gpio.gpio)
        group_index = int(offset / 32)
        phy_addr = board_gpioOffsetDic[str(group_index)]
        bit = offset % 32
        # config the devmem and write through to the hw
        reg = soc_get_tolerance_reg(phy_addr)
        reg.set_bit(bit)
        regs.append(reg)

    for reg in regs:
        reg.write()

        # export gpio
    for gpio in board_tolerance_gpio_table:
        openbmc_gpio.gpio_export(gpio.gpio, gpio.shadow)
        def_val = openbmc_gpio.gpio_get_value(gpio.gpio, change_direction=False)
        if def_val == 1:
            openbmc_gpio.gpio_set_value(gpio.gpio, 1)
        else:
            openbmc_gpio.gpio_set_value(gpio.gpio, 0)


def setup_board_gpio(soc_gpio_table, board_gpio_table, validate=True):
    soc = SocGPIOTable(soc_gpio_table)
    gpio_configured = []
    for gpio in board_gpio_table:
        try:
            soc.config_function(gpio.gpio, write_through=False)
            gpio_configured.append(gpio.gpio)
        except ConfigUnknownFunction as e:
            # not multiple-function GPIO pin
            pass
        except NotSmartEnoughException as e:
            logging.error('Failed to configure "%s" for "%s": %s'
                          % (gpio.gpio, gpio.shadow, str(e)))

    soc.write_to_hw()

    if validate:
        all_functions = set(soc.get_active_functions(refresh=True))
        for gpio in gpio_configured:
            if gpio not in all_functions:
                raise Exception('Failed to configure function "%s"' % gpio)

    for gpio in board_gpio_table:
        if not (gpio.gpio).startswith('GPIO'):
            continue
        openbmc_gpio.gpio_export(gpio.gpio, gpio.shadow)
        if gpio.value == GPIO_INPUT:
            continue
        elif gpio.value == GPIO_OUT_HIGH:
            openbmc_gpio.gpio_set_value(gpio.gpio, 1)
        elif gpio.value == GPIO_OUT_LOW:
            openbmc_gpio.gpio_set_value(gpio.gpio, 0)
        else:
            raise Exception('Invalid value "%s"' % gpio.value)
