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
import json
import argparse
import sys
import logging
import os
import subprocess
try:
    currentPath = os.getcwd()
    testPath = currentPath[0:currentPath.index('common')]
except Exception:
    testPath = '/tmp/tests/'
try:
    sys.path.insert(0, testPath + 'wedge/unittests/')
    import WedgeUtil
except Exception:
    pass
try:
    sys.path.insert(0, testPath + 'wedge100/unittests/')
    import Wedge100Util
except Exception:
    pass
try:
    sys.path.insert(0, testPath + 'cmm/unittests/')
    import CmmUtil
except Exception:
    pass
try:
    sys.path.insert(0, testPath + 'galaxy100/unittests/')
    import Galaxy100Util
except Exception:
    pass
try:
    sys.path.insert(0, testPath + 'fbtp/unittests/')
    import FbtpUtil
except Exception:
    pass
try:
    sys.path.insert(0, testPath + 'fby2/unittests/')
    import Fby2Util
except Exception:
    pass
try:
    sys.path.insert(0, testPath + 'fbttn/unittests/')
    import FbttnUtil
except Exception as e:
    pass
try:
    sys.path.insert(0, testPath + 'lightning/unittests/')
    import LightningUtil
except Exception as e:
    pass
try:
    sys.path.insert(0, testPath + 'minipack/unittests/')
    import MinipackUtil
except Exception as e:
    pass


class UnitTestUtil:
    """
    SensorData supports wedge, wedge100, cmm, galaxy100, fbttn, fby2, and fbtp
    """

    def importUtil(self, platformType):
        if platformType == 'wedge':
            return WedgeUtil.WedgeUtil()
        elif platformType == 'wedge100':
            return Wedge100Util.Wedge100Util()
        elif platformType == 'cmm':
            return CmmUtil.CmmUtil()
        elif platformType == 'galaxy100':
            return Galaxy100Util.Galaxy100Util()
        elif platformType == 'fbtp':
            return FbtpUtil.FbtpUtil()
        elif platformType == 'fby2':
            return Fby2Util.Fby2Util()
        elif platformType == 'fbttn':
            return FbttnUtil.FbttnUtil()
        elif platformType == 'lightning':
            return LightningUtil.LightningUtil()
        elif platformType == 'minipack':
            return MinipackUtil.MinipackUtil()
        else:
            raise Exception("Unsupported Platform")

    def JSONparser(self, file):
        """
        Get a json file and create a dictionary
        """
        with open(file) as data_file:
            data = json.load(data_file)
        return data

    def Argparser(self, names, platformTypes, helps, optional=None):
        parser = argparse.ArgumentParser()
        for i in range(len(names)):
            if platformTypes[i] is None:
                parser.add_argument(names[i], help=helps[i])
            elif optional and optional[i]:
                parser.add_argument(
                    names[i],
                    metavar=names[i],
                    type=platformTypes[i],
                    help=helps[i],
                    nargs='?')
            else:
                parser.add_argument(
                    names[i],
                    metavar=names[i],
                    type=platformTypes[i],
                    help=helps[i])
        return parser.parse_args()

    def Login(self, hostname, ssh):
        """
        Login to a given hostname
        """
        username = 'root'
        password = '0penBmc'
        if not ssh.login(hostname, username, password):
            print("Login [FAILED]")
        return

    def scp(self, path, hostname, Multiple, pexpect):
        """
        Scp file or files to the given hostname
        """
        dest = 'root@[' + hostname + ']:/tmp'
        password = '0penBmc'
        possible_expects = ['password:', pexpect.EOF]
        if Multiple:
            child = pexpect.spawn('scp -r ' + path + ' ' + dest)
        else:
            child = pexpect.spawn('scp ' + path + ' ' + dest)
        rc = child.expect(possible_expects, timeout=120)
        if rc == 0: # asked for password
            child.sendline(password)
            child.expect(pexpect.EOF)
        child.close()
        return

    def Login_through_proxy(self, headnode, bmcname, pexpect):
        headnode_password = 'password'
        bmc_password = '0penBmc'
        ssh_cmd = "sshpass -p {0} ssh -tt root@{1} sshpass -p {2} ssh root@{3}"
        ssh_cmd.format(headnode_password, headnode, bmc_password, bmcname)

    def scp_through_proxy(self, path, headnode, bmcname, Multiple, pexpect):
        headnode_password = 'password'
        bmc_password = '0penBmc'
        scpcmd = ""
        if Multiple:
            scpcmd = "scp -o 'ProxyCommand ssh root@{0} nc %h %p' -r {1} root@[{2}]:/tmp"
        else:
            scpcmd = "scp -o 'ProxyCommand ssh root@{0} nc %h %p' {1} root@[{2}]:/tmp"
        scpcmd = scpcmd.format(headnode, path, bmcname)
        child = pexpect.spawn(scpcmd)
        auth1 = child.expect(['password:', pexpect.EOF])
        if not auth1:
            child.sendline(headnode_password)
            auth2 = child.expect(['password:', pexpect.EOF])
            if not auth2:
                child.sendline(bmc_password)
                child.expect(pexpect.EOF)
        child.close()
        return

    def logger(self, mode):
        """
        Creates logger to log data and output to stdout
        """
        root = logging.getLogger()
        root.setLevel(mode)
        ch = logging.StreamHandler(sys.stdout)
        ch.setLevel(mode)
        formatter = logging.Formatter('%(levelname)s - %(message)s')
        ch.setFormatter(formatter)
        root.addHandler(ch)
        return root

    def run_shell_cmd(self, cmd):
        """Run the supplied command.
        """
        proc = subprocess.Popen(cmd,
                                shell=True,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        out, err = proc.communicate()
        if len(err) != 0:
            raise Exception(err)
        return out
