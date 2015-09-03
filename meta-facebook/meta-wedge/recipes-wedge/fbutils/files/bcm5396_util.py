#!/usr/bin/python -tt
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

from argparse import ArgumentParser
from bcm5396 import Bcm5396

def auto_long(x):
    return long(x, 0)

def auto_int(x):
    return int(x, 0)

def get_bcm(args):
    if args.spi:
        return Bcm5396(Bcm5396.SPI_ACCESS, cs=args.cs, clk=args.clk,
                       mosi=args.mosi, miso=args.miso,
                       verbose=args.verbose)
    else:
        return Bcm5396(Bcm5396.MDIO_ACCESS, mdc=args.mdc, mdio=args.mdio,
                       verbose=args.verbose)


def read_register(args):
    bcm = get_bcm(args)
    val = bcm.read(args.page, args.register, args.size)
    print('Read from BCM5396 ({}:{}.{}): {}'
          .format(hex(args.page), hex(args.register), args.size, hex(val)))

def write_register(args):
    bcm = get_bcm(args)
    val = bcm.write(args.page, args.register, args.value, args.size)
    print('Write to BCM5396 ({}.{}): {}'
          .format(hex(args.page), hex(args.register), hex(args.value)))

def register_parser(subparser):
    reg_parser = subparser.add_parser('register', help='Register IO')
    reg_sub = reg_parser.add_subparsers()

    read_parser = reg_sub.add_parser('read', help='read switch register')
    read_parser.set_defaults(func=read_register)
    read_parser.add_argument('page', type=auto_int,
                             help='The page of the register')
    read_parser.add_argument('register', type=auto_int,
                             help='The register to read from')
    read_parser.add_argument('size', type=auto_int,
                             help='Number of bytes',
                             choices=range(1, 9))

    write_parser = reg_sub.add_parser('write', help='write switch register')
    write_parser.set_defaults(func=write_register)
    write_parser.add_argument('page', type=auto_int,
                             help='The page oof the register')
    write_parser.add_argument('register', type=auto_int,
                              help='The register to write to')
    write_parser.add_argument('value', type=auto_long,
                             help='The value to write')
    write_parser.add_argument('size', type=auto_int,
                             help='Number of bytes',
                             choices=range(1, 9))


def dump_arl(args):
    bcm = get_bcm(args)
    arls = bcm.get_all_arls()
    print('All ARLs are:')
    for entry in arls:
        print(entry)

def arl_parser(subparser):
    dump_parser = subparser.add_parser('arl', help='dump all ARL entries')
    dump_parser.set_defaults(func=dump_arl)


def __parse_port_list(parm):
    '''Parse the port numbers to a list'''
    if len(parm) == 0:
        return []
    ports=[]
    for port in parm.split(','):
        idx = port.find('-')
        if idx == -1:
            p = int(port)
            if p < 0 or p > 15:
                raise Exception('Invalid port number %s' % p)
            # just one port
            ports.append(p)
        else:
            start = int(port[:idx])
            end = int(port[idx+1:])
            if start > end or start < 0 or end > 15:
                raise Exception('Invalid port range %s-%s' % (start, end))
            ports.extend(range(start, end + 1))
    return ports

def enable_vlan(args):
    bcm = get_bcm(args)
    bcm.vlan_ctrl(True)
    print('VLAN function is enabled.')

def disable_vlan(args):
    bcm = get_bcm(args)
    bcm.vlan_ctrl(False)
    print('VLAN function is disabled.')

def add_vlan(args):
    bcm = get_bcm(args)
    vid = args.vid
    fwd = sorted(__parse_port_list(args.fwd))
    untag = sorted(__parse_port_list(args.untag))
    spt = args.spt
    # make sure untag is subset of fwd
    if not set(untag).issubset(set(fwd)):
        raise Exception('Some untagged ports, %s, are not part of forward ports'
                        % (set(untag) - set(fwd)))
    bcm.add_vlan(vid, untag, fwd, spt)
    print('Added VLAN: %s' % vid)
    print('Ports in VLAN: %s' % sorted(fwd))
    print('Ports without VLAN tag: %s' % sorted(untag))

def remove_vlan(args):
    bcm = get_bcm(args)
    vid = args.vid
    bcm.remove_vlan(vid)
    print('Removed VLAN: %s' % vid)

def show_vlan(args):
    bcm = get_bcm(args)
    vid = args.vid
    vlan = bcm.get_vlan(vid)
    if not vlan['valid']:
        print('VLAN %s does not exist' % vid)
    else:
        print('VLAN: %s' % vid)
        print('Spanning tree index: %s' % vlan['spt'])
        print('Ports in VLAN: %s' % sorted(vlan['fwd']))
        print('Untagged ports in VLAN: %s' % sorted(vlan['untag']))

def set_port_vlan(args):
    bcm = get_bcm(args)
    bcm.vlan_set_port_default(args.port, args.vid, args.pri)
    print('Set VLAN default for port: %s' % args.port)
    print('Default VLAN: %s' % args.vid)
    print('Default priority %s' % args.pri)

def get_port_vlan(args):
    bcm = get_bcm(args)
    port = bcm.vlan_get_port_default(args.port)
    print('Get VLAN default for port: %s' % args.port)
    print('Default VLAN: %s' % port['vid'])
    print('Default priority: %s' % port['priority'])

def vlan_parser(subparser):
    UNTAG_DEFAULT = ''
    SPT_DEFAULT = 0
    PRI_DEFAULT = 0

    vlan_parser = subparser.add_parser('vlan', help='Manage vlan function')
    vlan_sub = vlan_parser.add_subparsers()

    add_parser = vlan_sub.add_parser('add', help='Add or modify a VLAN entry')
    add_parser.add_argument('vid', type=int, help='The VLAN ID')
    add_parser.add_argument('fwd', type=str,
                            help='Ports belonging to this VLAN. i.e. 1,4,5-8')
    add_parser.add_argument('untag', type=str, default=UNTAG_DEFAULT, nargs='?',
                            help='Ports that do not add VLAN tag. i.e. 1,4,5-8'
                            ' (default: all ports are tagged)')
    add_parser.add_argument('spt', type=int, default=SPT_DEFAULT, nargs='?',
                            help='Spanning tree index (default: %s)'
                            % SPT_DEFAULT)
    add_parser.set_defaults(func=add_vlan)

    remove_parser = vlan_sub.add_parser('remove', help='Remove a VLAN entry')
    remove_parser.add_argument('vid', type=int, help='The VLAN ID')
    remove_parser.set_defaults(func=remove_vlan)

    show_parser = vlan_sub.add_parser('show', help='Show a VLAN entry')
    show_parser.add_argument('vid', type=int, help='The VLAN ID')
    show_parser.set_defaults(func=show_vlan)

    enable_parser = vlan_sub.add_parser('enable', help='Enable VLAN function')
    enable_parser.set_defaults(func=enable_vlan)

    disable_parser = vlan_sub.add_parser('disable', help='Enable VLAN function')
    disable_parser.set_defaults(func=disable_vlan)

    port_parser = vlan_sub.add_parser('port',
                                      help='Set/Get VLAN default for a port')
    port_sub = port_parser.add_subparsers()
    set_port = port_sub.add_parser('set', help='Set VLAN default for a port')
    set_port.add_argument('port', type=int, help='The port number (0..16)')
    set_port.add_argument('vid', type=int,
                          help='The default VLAN for this port')
    set_port.add_argument('pri', type=int, default=PRI_DEFAULT, nargs='?',
                          help='The default priority for this port '
                          '(default: %s)' % PRI_DEFAULT)
    set_port.set_defaults(func=set_port_vlan)

    get_port = port_sub.add_parser('get', help='Get VLAN default for a port')
    get_port.add_argument('port', type=int, help='The port number (0..16)')
    get_port.set_defaults(func=get_port_vlan)

def access_parser(ap):
    SPI_CS_DEFAULT = 68
    SPI_CLK_DEFAULT = 69
    SPI_MOSI_DEFAULT = 70
    SPI_MISO_DEFAULT = 71

    MDIO_MDC_DEFAULT = 6
    MDIO_MDIO_DEFAULT = 7

    spi_group = ap.add_argument_group('SPI Access')
    spi_group.add_argument('--spi', action='store_true',
                           help='Access through SPI.')
    spi_group.add_argument('--cs', type=int, default=SPI_CS_DEFAULT,
                           help='The GPIO number for SPI CS pin (default: %s)'
                           % SPI_CS_DEFAULT)
    spi_group.add_argument('--clk', type=int, default=SPI_CLK_DEFAULT,
                           help='The GPIO number for SPI CLK pin (default: %s)'
                           % SPI_CLK_DEFAULT)
    spi_group.add_argument('--mosi', type=int, default=SPI_MOSI_DEFAULT,
                           help='The GPIO number for SPI MOSI pin (default: %s)'
                           % SPI_MOSI_DEFAULT)
    spi_group.add_argument('--miso', type=int, default=SPI_MISO_DEFAULT,
                           help='The GPIO number for SPI MISO pin (default: %s)'
                           % SPI_MISO_DEFAULT)
    mdio_group = ap.add_argument_group('MDIO Access (default)')
    mdio_group.add_argument('--mdc', type=int, default=MDIO_MDC_DEFAULT,
                           help='The GPIO number for MDC pin (default: %s)'
                           % MDIO_MDC_DEFAULT)
    mdio_group.add_argument('--mdio', type=int, default=MDIO_MDIO_DEFAULT,
                           help='The GPIO number for MDIO pin (default: %s)'
                           % MDIO_MDIO_DEFAULT)
    return ap


if __name__ == '__main__':
    ap = ArgumentParser()
    ap.add_argument('-v', '--verbose', action='store_true',
                    help='Dump the switch page, register, and value '
                    'for each operation')

    access_parser(ap)

    subparsers = ap.add_subparsers()
    register_parser(subparsers)
    arl_parser(subparsers)
    vlan_parser(subparsers)
    args = ap.parse_args()

    args.func(args)
