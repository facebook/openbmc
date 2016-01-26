#
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
from __future__ import print_function


import subprocess
import time


class Bcm5396MDIO:
    '''The class to access BCM5396 through MDIO intf'''
    MDIO_CMD = 'mdio-bb'

    PHYADDR = 0x1E

    ACCESS_CTRL_REG = 16
    IO_CTRL_REG = 17
    STATUS_REG = 18
    DATA0_REG = 24
    DATA1_REG = 25
    DATA2_REG = 26
    DATA3_REG = 27

    def __init__(self, mdc, mdio):
        self.mdc = mdc
        self.mdio = mdio
        self.page = -1

    def __io(self, op, reg, val=0):
        cmd = '%s -p -c %s -d %s %s %s %s' \
              % (self.MDIO_CMD, self.mdc, self.mdio, op, str(self.PHYADDR),
                 str(reg))
        if op == 'write':
            cmd += ' %s' % val
        out = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE)\
                        .communicate()[0]
        if op == 'write':
            return val
        # need to parse the result for read
        rc = 0
        for line in out.split('\n'):
            if not line.startswith('Read:'):
                continue
            rc = int(line.split(':')[1], 0)
        return rc

    def __read_mdio(self, reg):
        return self.__io('read', reg)

    def __write_mdio(self, reg, val):
        return self.__io('write', reg, val)

    def __set_page(self, page):
        if self.page == page:
            return
        # Write MII register ACCESS_CTRL_REG:
        # set bit 0 as "1" to enable MDIO access
        # set "page number to bit 15:8
        val = 0x1 | ((page & 0xff) << 8)
        self.__write_mdio(self.ACCESS_CTRL_REG, val)
        self.page = page

    def __wait_for_done(self):
        # Read MII register IO_CTRL_REG:
        # Check op_code = "00"
        while (self.__read_mdio(self.IO_CTRL_REG) & 0x3):
            time.sleep(0.010)   # 10ms

    def read(self, page, reg, n_bytes):
        self.__set_page(page)
        # Write MII register IO_CTRL_REG:
        # set "Operation Code as "00"
        # set "Register Address" to bit 15:8
        val = 0x00 | ((reg & 0xff) << 8)
        self.__write_mdio(self.IO_CTRL_REG, val)
        # Write MII register IO_CTRL_REG:
        # set "Operation Code as "10"
        # set "Register Address" to bit 15:8
        val = 0x2 | ((reg & 0xff) << 8)
        self.__write_mdio(self.IO_CTRL_REG, val)
        self.__wait_for_done()
        # Read MII register DATA0_REG for bit 15:0
        val = long(self.__read_mdio(self.DATA0_REG))
        # Read MII register DATA1_REG for bit 31:16
        val |= self.__read_mdio(self.DATA1_REG) << 16
        # Read MII register DATA2_REG for bit 47:32
        val |= self.__read_mdio(self.DATA2_REG) << 32
        # Read MII register DATA3_REG for bit 63:48
        val |= self.__read_mdio(self.DATA3_REG) << 48
        return val

    def write(self, page, reg, val, n_bytes):
        self.__set_page(page)
        # Write MII register DATA0_REG for bit 15:0
        self.__write_mdio(self.DATA0_REG, val & 0xFFFF)
        # Write MII register DATA1_REG for bit 31:16
        self.__write_mdio(self.DATA1_REG, (val >> 16) & 0xFFFF)
        # Write MII register DATA2_REG for bit 47:32
        self.__write_mdio(self.DATA2_REG, (val >> 32) & 0xFFFF)
        # Write MII register DATA3_REG for bit 63:48
        self.__write_mdio(self.DATA3_REG, (val >> 48) & 0xFFFF)
        # Write MII register IO_CTRL_REG:
        # set "Operation Code as "00"
        # set "Register Address" to bit 15:8
        val = 0x00 | ((reg & 0xff) << 8)
        self.__write_mdio(self.IO_CTRL_REG, val)
        # Write MII register IO_CTRL_REG:
        # set "Operation Code as "01"
        # set "Register Address" to bit 15:8
        val = 0x1 | ((reg & 0xff) << 8)
        self.__write_mdio(self.IO_CTRL_REG, val)
        self.__wait_for_done()


class Bcm5396SPI:
    '''The class to access BCM5396 through SPI interface'''
    SPI_CMD = 'spi-bb'

    READ_CMD = 0x60
    WRITE_CMD = 0x61

    SPI_STS_DIO = 0xF0
    SPI_STS_REG = 0xFE
    SPI_STS_REG_RACK = 0x1 << 5
    SPI_STS_REG_SPIF = 0x1 << 7
    PAGE_REG = 0xFF

    def __init__(self, cs, clk, mosi, miso):
        self.cs = cs
        self.clk = clk
        self.mosi = mosi
        self.miso = miso
        self.page = -1

    def __bytes2val(self, values):
        # LSB first, MSB last
        pos = 0
        result = 0L
        for byte in values:
            if type(byte) is str:
                byte = int(byte, 16)
            if byte > 255:
                raise Exception('%s is not a byte in the list %s'\
                                % (byte, values))
            result |= byte << pos
            pos += 8
        return result

    def __val2bytes(self, value, n):
        result = []
        for _ in range(n):
            result.append(value & 0xFF)
            value >>= 8
        if value > 0:
            raise Exception('Value, %s, is too large for %s bytes'
                            % (value, n))
        return result

    def __io(self, bytes_to_write, to_read=0):
        # TODO: check parameters
        cmd = '%s -s %s -S low -c %s -o %s -i %s '\
              % (self.SPI_CMD, self.cs, self.clk, self.mosi, self.miso)
        if len(bytes_to_write):
            write_cmd = '-w %s %s '\
                        % (len(bytes_to_write) * 8,
                           ' '.join([str(byte) for byte in bytes_to_write]))
        else:
            write_cmd = ''
        if to_read:
            # spi-bb will first return the exact number of bits used for
            # writing. So, total number of bits to read should also include
            # the number of bits written.
            cmd += '-r %s ' % str((len(bytes_to_write) + to_read) * 8)
        cmd += write_cmd
        rc = 0L
        out = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE)\
                        .communicate()[0]
        if to_read:
            # need to parse the result
            for line in out.split('\n'):
                if not line.startswith('Read'):
                    continue
                res = line.split(':')[1]
                rc = self.__bytes2val(res.split()[len(bytes_to_write):])
                break
        return rc

    def __set_page(self, page):
        page &= 0xff
        if self.page == page:
            return
        self.__io([self.WRITE_CMD, self.PAGE_REG, page])
        self.page = page

    def __read_spi_reg(self, reg):
        reg &= 0xFF
        return self.__io([self.READ_CMD, reg], 1)

    def __read_spi_sts(self):
        return self.__read_spi_reg(self.SPI_STS_REG)

    def __read_spi_dio(self):
        return self.__read_spi_reg(self.SPI_STS_DIO)

    def read(self, page, reg, n_bytes):
        '''Read a register value from a page.'''
        if n_bytes > 8:
            print('TODO to support reading more than 8 bytes')
            return 0
        if page > 0xff or reg > 0xff:
            print('Page and register must be <= 255')
            return 0
        try:
            self.__set_page(page)
            self.__io([self.READ_CMD, reg], 1)
            while True:
                # check sts
                sts = self.__read_spi_sts()
                if sts & self.SPI_STS_REG_RACK:
                    break
            bytes = []
            for _ in range(n_bytes):
                bytes.append(self.__read_spi_dio())
        except Exception as e:
            print(e)
        return self.__bytes2val(bytes)

    def write(self, page, reg, val, n_bytes):
        '''Write a value as n bytes to a register on a page.'''
        if page > 0xff or reg > 0xff:
            print('Page and register must be <= 255')
            return
        bytes = self.__val2bytes(val, n_bytes)
        if len(bytes) > 8:
            print('TODO to support writing more than 8 bytes')
            return
        bytes = [self.WRITE_CMD, reg] + bytes
        try:
            self.__set_page(page)
            self.__io(bytes)
        except Exception as e:
            print(e)


class Bcm5396:
    '''The class for BCM5396 Switch'''

    MDIO_ACCESS = 0
    SPI_ACCESS = 1

    def __init__(self, access, verbose=False, **kwargs):
        self.verbose = verbose
        if access == self.MDIO_ACCESS:
            self.access = Bcm5396MDIO(**kwargs)
        else:
            self.access = Bcm5396SPI(**kwargs)

    def write(self, page, reg, value, n_bytes):
        if self.verbose:
            print('WRITE {:2x} {:2x} {:2x} '.format(page, reg, n_bytes), end='')
            bytes = '{:2x}'.format(value)
            print([bytes[i:i+2] for i in range(0, len(bytes), 2)][-n_bytes:])
        return self.access.write(page, reg, value, n_bytes)

    def read(self, page, reg, n_bytes):
        if self.verbose:
            print('READ {:2x} {:2x} {:2x} '.format(page, reg, n_bytes), end='')
        result = self.access.read(page, reg, n_bytes)
        if self.verbose:
            bytes = '{:2x}'.format(result)
            print([bytes[i:i+2] for i in range(0, len(bytes), 2)][-n_bytes:])
        return result

    def __add_remove_vlan(self, add, vid, untag, fwd, spt):
        VLAN_PAGE = 0x5
        CTRL_ADDR = 0x60
        CTRL_START_DONE = (0x1 << 7)
        VID_ADDR = 0x61
        ENTRY_ADDR = 0x63

        fwd_map = self.__ports2portmap(fwd)
        untag_map = self.__ports2portmap(untag)

        # mark it as write and stop the previous action
        ctrl = 0
        self.write(VLAN_PAGE, CTRL_ADDR, ctrl, 1)
        # write entry
        if (add):
            entry = 0x1L | ((spt & 0x1F) << 1) \
                | (fwd_map << 6) | (untag_map << 23)
        else:
            entry = 0x0L
        self.write(VLAN_PAGE, ENTRY_ADDR, entry, 8)
        # write vid as the index
        self.write(VLAN_PAGE, VID_ADDR, vid & 0xFFF, 2)
        # start the write
        ctrl = CTRL_START_DONE
        self.write(VLAN_PAGE, CTRL_ADDR, ctrl, 1)
        while True:
            ctrl = self.read(VLAN_PAGE, CTRL_ADDR, 1)
            if not (ctrl & CTRL_START_DONE):
                # done
                break
            time.sleep(0.010)   # 10ms

    def add_vlan(self, vid, untag, fwd, spt=0):
        return self.__add_remove_vlan(True, vid, untag, fwd, spt)

    def remove_vlan(self, vid):
        return self.__add_remove_vlan(False, vid, [], [], 0)

    def get_vlan(self, vid):
        VLAN_PAGE = 0x5
        CTRL_ADDR = 0x60
        CTRL_START_DONE = (0x1 << 7)
        CTRL_READ = 0x1
        VID_ADDR = 0x61
        ENTRY_ADDR = 0x63

        # mark it as read and stop the previous action
        ctrl = CTRL_READ
        self.write(VLAN_PAGE, CTRL_ADDR, ctrl, 1)
        # write the vid as the index
        self.write(VLAN_PAGE, VID_ADDR, vid & 0xFFF, 2)
        # start the read
        ctrl = CTRL_READ|CTRL_START_DONE
        self.write(VLAN_PAGE, CTRL_ADDR, ctrl, 1)
        while True:
            ctrl = self.read(VLAN_PAGE, CTRL_ADDR, 1)
            if not (ctrl & CTRL_START_DONE):
                # done
                break
            time.sleep(0.010)   # 10ms
        entry = self.read(VLAN_PAGE, ENTRY_ADDR, 8)
        res = {}
        res['valid'] = True if entry & 0x1 else False
        res['spt'] = (entry >> 1) & 0x1f
        res['fwd'] = self.__portmap2ports((entry >> 6) & 0x1ffff)
        res['untag'] = self.__portmap2ports((entry >> 23) & 0x1ffff)
        return res

    def __portmap2ports(self, port_map):
        return list(set([port if port_map & (0x1 << port) else None
                         for port in range (0, 17)])
                    - set([None]))

    def __ports2portmap(self, ports):
        port_map = 0
        for port in ports:
            port_map |= (0x1 << port)
        return port_map & 0x1FFFF

    def __parse_arl_result(self, vid, result):
        is_bitset = lambda bit: True if result & (0x1 << bit) else False
        if not is_bitset(3):
            return None
        res = {}
        # parse vid first
        res['vid'] = (vid >> 48) & 0xfff
        mac_val = vid & 0xffffffffffffL
        mac_list = []
        for pos in range(5, -1, -1):
            mac_list.append('{:02x}'.format((mac_val >> (pos * 8)) & 0xff))
        res['mac'] = ':'.join(mac_list)
        if mac_val & (0x1 << 40):
            res['ports'] = self.__portmap2ports((result >> 6) & 0xffff)
        else:
            res['ports'] = [(result >> 6) & 0xf]
        res['static'] = is_bitset(5)
        res['age'] = is_bitset(4)
        res['valid'] = is_bitset(3)
        res['priority'] = result & 0x7
        return res

    def get_all_arls(self):
        ARL_PAGE = 0x5
        SEARCH_CTRL_ADDR = 0x30
        SEARCH_CTRL_START_DONE = (0x1 << 7)
        SEARCH_CTRL_SR_VALID = (0x1)

        VID0_ADDR = 0x33
        RESULT0_ADDR = 0x3B
        VID1_ADDR = 0x40
        RESULT1_ADDR = 0x48

        all = []
        # write START to search control
        ctrl = SEARCH_CTRL_START_DONE
        self.write(ARL_PAGE, SEARCH_CTRL_ADDR, ctrl, 1)
        while True:
            ctrl = self.read(ARL_PAGE, SEARCH_CTRL_ADDR, 1)
            if not (ctrl & SEARCH_CTRL_START_DONE):
                # Done
                break
            if not (ctrl & SEARCH_CTRL_SR_VALID):
                # result is not ready, sleep and retry
                time.sleep(0.010)   # 10ms
                continue
            for vid_addr, result_addr in [[VID1_ADDR, RESULT1_ADDR],
                                          [VID0_ADDR, RESULT0_ADDR]]:
                vid = self.read(ARL_PAGE, vid_addr, 8)
                result = self.read(ARL_PAGE, result_addr, 4)
                one = self.__parse_arl_result(vid, result)
                if one:
                    all.append(one)
        return all

    def vlan_ctrl(self, enable):
        VLAN_CTRL_PAGE = 0x34
        VLAN_CTRL0_REG = 0x0
        VLAN_CTRL0_B_EN_1QVLAN = 0x1 << 7

        ctrl = self.read(VLAN_CTRL_PAGE, VLAN_CTRL0_REG, 1)
        need_write = False
        if enable:
            if not ctrl & VLAN_CTRL0_B_EN_1QVLAN:
                need_write = True;
                ctrl |=  VLAN_CTRL0_B_EN_1QVLAN
        else:
            if ctrl & VLAN_CTRL0_B_EN_1QVLAN:
                need_write = True;
                ctrl &=  (~VLAN_CTRL0_B_EN_1QVLAN) & 0xFF
        if need_write:
            self.write(VLAN_CTRL_PAGE, VLAN_CTRL0_REG, ctrl, 1)

    def vlan_set_port_default(self, port, vid, pri=0):
        VLAN_PORT_PAGE = 0x34
        VLAN_PORT_REG_BASE = 0x10

        if port < 0 or port > 16:
            raise Exception('Invalid port number %s' % port)
        if pri < 0 or pri > 7:
            raise Exception('Invalid priority %s' % pri)
        if vid < 0 or vid > 0xFFF:
            raise Exception('Invalid VLAN %s' % vid)
        reg = VLAN_PORT_REG_BASE + port * 2
        ctrl = (pri << 13) | vid
        self.write(VLAN_PORT_PAGE, reg, ctrl, 2)

    def vlan_get_port_default(self, port):
        VLAN_PORT_PAGE = 0x34
        VLAN_PORT_REG_BASE = 0x10

        if port < 0 or port > 16:
            raise Exception('Invalid port number %s' % port)
        reg = VLAN_PORT_REG_BASE + port * 2
        val = self.read(VLAN_PORT_PAGE, reg, 2)
        res = {}
        res['priority'] = (val >> 13) & 0x7
        res['vid'] = val & 0xFFF
        return res
