# Copyright 2004-present Facebook. All rights reserved.
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

import subprocess
import struct
import sys


AT93C46 = 'at93c46'
AT93C56 = 'at93c56'
AT93C66 = 'at93c66'
AT93C86 = 'at93c86'


class VerboseLogger:
    def __init__(self, verbose=False):
        self.verbose = verbose

    def _verbose_print(self, caption, bytestream=None):
        '''
        Print a bytestream to stdout if verbose is enabled.
        '''
        if self.verbose:
            if bytestream is not None:
                sys.stderr.write(
                    "{}: {}\n" .format(
                        caption, " ".join(['{:02X}'.format(ord(x))
                                           for x in bytestream])))
            else:
                sys.stderr.write("{}\n".format(caption))


class AT93CX6SPI(VerboseLogger):
    '''The class to access AT93CX6 through SPI intf'''
    SPI_CMD = 'spi-bb'

    def __init__(self, bus_width, gpio_cs, gpio_ck, gpio_do, gpio_di,
                 model, verbose=False):
        addr_bits_map = {
            AT93C46 : 6,
            AT93C56 : 8,
            AT93C66 : 8,
            AT93C86 : 10,
        }
        if bus_width != 8 and bus_width != 16:
            raise Exception("Invalid bus width for AT93CX6!")
        if model not in addr_bits_map:
            raise Exception("Invalid model '%s'" % model)

        self.bus_width = bus_width
        self.gpio_cs = gpio_cs
        self.gpio_ck = gpio_ck
        self.gpio_do = gpio_do
        self.gpio_di = gpio_di
        self.verbose = verbose

        self.addr_bits = addr_bits_map[model] \
                         + (0 if self.bus_width == 16 else 1)
        self.addr_mask = (1 << self.addr_bits) - 1

    def __shift(self, bytestream, value):
        '''
        Shift an entire byte stream by value bits.
        '''
        binary = "".join(['{:08b}'.format(ord(x)) for x in bytestream])
        if value > 0:
            binary = binary[value:] + '0' * value
        else:
            binary = '0' * (-value) + binary[:value]
        return "".join([chr(int(binary[x:x+8],2))
                        for x in range(0, len(binary), 8)])

    def __io(self, op, addr, data=None):
        '''
        Perform an IO operation against the EEPROM
        '''
        write_bits = self.addr_bits + 3
        if data is not None:
            # If giving data, we are doing a write command so
            # no need to read any data.
            write_bits = write_bits + self.bus_width
            read_bits = 0
        else:
            # If not giving data, we are doing either a read
            # command or a set command, so read the result.
            # We pad with an extra bit due to a dummy bit introduced
            # by a delay for address decoding on chip.
            read_bits = self.addr_bits + 4 + self.bus_width

        # Format the command itself
        instruction = addr & self.addr_mask
        instruction = instruction | ((0x4 | (op & 0x3)) << self.addr_bits)
        if data is not None:
            if self.bus_width == 16:
                write_data = struct.pack(">HH", instruction, data & 0xFFFF)
            else:
                write_data = struct.pack(">HB", instruction, data & 0xFF)
        else:
            write_data = struct.pack(">H", instruction)
        write_data = self.__shift(write_data, 16 - (self.addr_bits + 3))

        self._verbose_print("Write data", write_data)

        # Run the command with the bitbang driver
        if read_bits > 0:
            data_portion = "-r {} -w {}".format(read_bits, write_bits)
        else:
            data_portion = "-w {}".format(write_bits)

        cmd = "{} -s {} -c {} -o {} -i {} -b {}".format(
            self.SPI_CMD, self.gpio_cs, self.gpio_ck, self.gpio_do,
            self.gpio_di, data_portion
        )

        self._verbose_print("Command: {}".format(cmd))

        out = subprocess.Popen(cmd.split(),
                               stdout=subprocess.PIPE,
                               stdin = subprocess.PIPE)\
                        .communicate(input=write_data)

        # Format the response
        read_data = self.__shift(out[0], self.addr_bits + 4)
        if self.bus_width == 16:
            read_data = read_data[:2]
            self._verbose_print("Read data", read_data)
            return struct.unpack(">H", read_data)[0]
        else:
            read_data = read_data[:1]
            self._verbose_print("Read data", read_data)
            return struct.unpack(">B", read_data)[0]

    def read(self, addr):
        return self.__io(0x2, addr)

    def ewen(self):
        self.__io(0x0, 0x3 << (self.addr_bits - 2))

    def erase(self, addr):
        self.__io(0x3, addr)

    def write(self, addr, data):
        self.__io(0x1, addr, data)

    def eral(self):
        self.__io(0x0, 0x2 << (self.addr_bits - 2))

    def wral(self, data):
        self.__io(0x0, 0x1 << (self.addr_bits - 2), data)

    def ewds(self):
        self.__io(0x0, 0x0)


class AT93CX6(VerboseLogger):
    '''
    The class which handles accessing memory on the AT93CX6 chip.
    '''

    def __init__(self, bus_width, gpio_cs, gpio_ck, gpio_do, gpio_di,
                 byte_swap, model=AT93C46, verbose=False):
        mem_size_map = {
            # in bytes
            AT93C46 : 128,
            AT93C56 : 256,
            AT93C66 : 512,
            AT93C86 : 2048,
        }
        self.bus_width = bus_width
        self.verbose = verbose
        self.byte_swap = byte_swap
        self.model = model
        self.memory_size = mem_size_map[model]

        self.spi = AT93CX6SPI(bus_width=bus_width, gpio_cs=gpio_cs,
                              gpio_ck=gpio_ck, gpio_do=gpio_do,
                              gpio_di=gpio_di, model=model,
                              verbose=verbose)

    def __swap(self, value):
        '''
        Swap bytes for a 16-bit integer if instructed to do so.
        '''
        if self.bus_width == 16:
            if self.byte_swap:
                return ((value >> 8) & 0xFF) | ((value << 8) & 0xFF00)
            else:
                return value
        else:
            return value

    def get_memory_size(self):
        return self.memory_size

    def erase(self, offset=None, limit=None):
        '''
        Erase the chip.
        '''
        if offset is None:
            offset = 0
        if limit is None:
            limit = self.memory_size

        if offset < 0 or offset + limit > self.memory_size:
            raise Exception("Erase would be out of bounds!")
        if self.bus_width == 16 and \
           ((offset & 1) != 0 or ((offset + limit) & 1) != 0):
            raise Exception("Erase can't start or end on odd boundary in "
                            "16-bit mode!")

        if offset == 0 and limit == self.memory_size:
            # Special case when we are erasing the entire chip
            self.spi.ewen()
            self.spi.eral()
            self.spi.ewds()

            self._verbose_print("Erased entire chip")
        else:
            # Regular case
            if self.bus_width == 16:
                real_offset = offset / 2
                real_limit = limit / 2
            else:
                real_offset = offset
                real_limit = limit

            self.spi.ewen()
            for addr in range(real_offset, real_offset + real_limit):
                self.spi.erase(addr)
            self.spi.ewds()

            self._verbose_print("Erased {} bytes from offset {}"
                                .format(limit, offset))

    def read(self, offset=None, limit=None):
        '''
        Read the chip into a memory buffer.
        '''
        if offset is None:
            offset = 0
        if limit is None:
            limit = self.memory_size

        if offset < 0 or offset + limit > self.memory_size:
            raise Exception("Read would be out of bounds!")
        if self.bus_width == 16 and \
           ((offset & 1) != 0 or ((offset + limit) & 1) != 0):
            raise Exception("Read can't start or end on odd boundary in 16-bit "
                            "mode!")

        output = ""
        if self.bus_width == 16:
            real_offset = offset / 2
            real_limit = limit / 2
            pack_instruction = "=H"
        else:
            real_offset = offset
            real_offset
            pack_instruction = "=B"

        for addr in range(real_offset, real_offset + real_limit):
            output = output + struct.pack(pack_instruction,
                                          self.__swap(self.spi.read(addr)))

        self._verbose_print("Read {} bytes from offset {}".format(limit, offset)
                            , output)

        return output

    def write(self, data, offset=None):
        '''
        Write a memory buffer to the chip.
        '''
        if offset is None:
            offset = 0

        if offset < 0 or offset + len(data) > self.memory_size:
            raise Exception("Write would be out of bounds!")
        if self.bus_width == 16 and \
           ((offset & 1) != 0 or ((offset + len(data)) & 1) != 0):
            raise Exception("Write can't start or end on odd boundary in "
                            "16-bit mode!")

        if self.bus_width == 16:
            offset_divisor = 2
            pack_instruction = "=H"
        else:
            offset_divisor = 1
            pack_instruction = "=B"

        self.spi.ewen()
        for addr in range(offset, offset + len(data), offset_divisor):
            actual_addr = addr / offset_divisor
            value = self.__swap(struct.unpack(
                pack_instruction, data[(addr - offset):(addr - offset)
                                       + offset_divisor])[0])

            self.spi.erase(actual_addr)
            self.spi.write(actual_addr, value)
        self.spi.ewds()

        self._verbose_print("Wrote {} bytes from offset {}"
                            .format(len(data), offset), data)
