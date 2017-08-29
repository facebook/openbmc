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





import subprocess
import logging


class PhyMemory(object):
    def __init__(self, addr, name=''):
        self.addr = addr
        self.name = name
        self.write_pending = False
        self.value = 0

    def __del__(self):
        if self.write_pending:
            logging.warning('Value (0x%x) is not wrote back to address (0x%x)'
                            % (self.value, self.addr))

    def __str__(self):
        if self.name == '':
            return '0x%x' % self.addr
        else:
            return self.name

    def _read_hw(self):
        if self.write_pending:
            raise Exception('Value (0x%x) is not wrote back to address (0x%x) '
                            'before reading HW' % (self.value, self.addr))
        cmd = ['devmem', '0x%x' % self.addr]
        out = subprocess.check_output(cmd)
        self.value = int(out, 16)
        logging.debug('Read from %s @0x%x, got value (0x%x)'
                      % (str(self), self.addr, self.value))

    def read(self, refresh=True):
        if refresh:
            self._read_hw()
        return self.value

    def write(self, force=False):
        if not force and not self.write_pending:
            return
        cmd = ['devmem', '0x%x' % self.addr, '32', '0x%x' % self.value]
        subprocess.check_call(cmd)
        self.write_pending = False
        logging.debug('Wrote to %s address @0x%x with value (0x%x)'
                      % (str(self), self.addr, self.value))

    def set_bit(self, bit, write_through=True):
        assert 0 <= bit <= 31
        self.value |= 1 << bit
        self.write_pending = True
        logging.debug('Set bit %s[%d] (0x%x)' % (str(self), bit, self.value))
        if write_through:
            self.write()

    def clear_bit(self, bit, write_through=True):
        assert 0 <= bit <= 31
        self.value &= ~(1 << bit)
        self.write_pending = True
        logging.debug('Clear bit %s[%d] (0x%x)' % (str(self), bit, self.value))
        if write_through:
            self.write()

    def is_bit_set(self, bit, refresh=False):
        assert 0 <= bit <= 31
        self.read(refresh=refresh)
        rc = True if self.value & (0x1 << bit) else False
        logging.debug('Test bit %s[%d](0x%x): %s'
                      % (str(self), bit, self.value, rc))
        return rc

    def bits_value(self, bits, refresh=False):
        self.read(refresh=refresh)
        value = 0
        for bit in sorted(bits, reverse=True):
            assert 0 <= bit <= 31
            value = (value << 1) | ((self.value >> bit) & 0x1)
        logging.debug('%s%s is 0x%x (0x%x)'
                      % (str(self), bits, value, self.value))
        return value
