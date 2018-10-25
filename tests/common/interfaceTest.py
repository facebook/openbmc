#!/usr/bin/env python
#
# Copyright 2018-present Facebook. All Rights Reserved.
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

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import subprocess
import sys
import unitTestUtil
import logging


def get_ip4_address(ifname):
    """
    Get IPv4 address of a given interface
    """
    f = subprocess.Popen(['ip', 'addr', 'show', ifname.encode('utf-8')],
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    out, err = f.communicate()
    if (len(out) == 0 or len(err) != 0):
        raise Exception("Device " + ifname.encode('utf-8') + " does not exist [FAILED]")
    out = out.decode('utf-8')
    ipv4 = out.split("inet ")[1].split("/")[0]
    logger.debug("Got ip address for " + str(ifname))
    return ipv4


def get_ip6_address(ifname):
    """
    Get IPv6 address of a given interface
    """
    f = subprocess.Popen(['ip', 'addr', 'show', ifname.encode('utf-8')],
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    out, err = f.communicate()
    if (len(out) == 0 or len(err) != 0):
        raise Exception("Device " + ifname.encode('utf-8') + " does not exist [FAILED]")
    out = out.decode('utf-8')
    ipv6 = out.split("inet6 ")[1].split("/")[0]
    logger.debug("Got ip address for " + str(ifname))
    return ipv6


def get_ifusb0_ip6_address(ifname):
    """
    Get IPv6 address of a given interface
    """
    f = subprocess.Popen(['ip', 'addr', 'show', ifname.encode('utf-8')],
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    out, err = f.communicate()
    if (len(out) == 0 or len(err) != 0):
        raise Exception("Device " + ifname.encode('utf-8') + " does not exist [FAILED]")
    out = out.decode('utf-8')
    ipv6 = out.split("inet6 ")[1].split("/")[0]
    usb0 = "".join(ipv6) + "%usb0"
    logger.debug("Got ip address for " + str(ifname))
    return usb0


def pingConnection(ifname, ver):
    """
    Test connection of a given interface and version of IP address
    """
    cmd = ""
    logger.debug("getting ip address for " + str(ifname))
    if ifname == "usb0":
        ip = get_ifusb0_ip6_address(ifname)
        cmd = "ping6 -c 1 -q -w 1 " + ip
    elif int(ver) == 4:
        ip = get_ip4_address(ifname)
        cmd = "ping -c 1 -q -w 1 " + ip
    elif int(ver) == 6:
        ip = get_ip6_address(ifname)
        cmd = "ping6 -c 1 -q -w 1 " + ip

    if cmd != "":
        logger.debug("Executing: " + str(cmd))
        f = subprocess.Popen(cmd, shell=True,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        err = f.communicate()[1]
        if len(err) > 0:
            raise Exception(err)
    exitcode = f.returncode
    if exitcode == 0:
        print('Connection for ' + ifname + ' v' + str(ver) + " [PASSED]")
        sys.exit(0)
    else:
        print('Connection for ' + ifname + ' v' + str(ver) + " [FAILED]")
        sys.exit(1)


if __name__ == "__main__":
    """
    Input to this file should look like the following:
    python interfaceTest.py ifname0=eth0 ver0=4 ifname1=eth0 ver1=6
    """
    util = unitTestUtil.UnitTestUtil()
    logger = util.logger(logging.WARN)
    try:
        args = util.Argparser(['ifname', 'ver', '--verbose'], [str, int, None],
                              ['an interface name', 'a version number',
                              'output all steps from test with mode options: DEBUG, INFO, WARNING, ERROR'])
        if args.verbose is not None:
            logger = util.logger(args.verbose)
        ifname = args.ifname
        ver = str(args.ver)
        pingConnection(ifname, ver)
    except Exception as e:
        if type(e) == IndexError:
            print('No IPv' + ver + ' address for ' + ifname.encode('utf-8') +
                  ' interface [FAILED]')
        else:
            print("interface Test [FAILED]")
            print("Error: " + str(e))
        sys.exit(1)
