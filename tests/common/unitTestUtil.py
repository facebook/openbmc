from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import json
import argparse
import sys
import logging
import os
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
    sys.path.insert(0, testPath + 'fbttn/unittests/')
    import FbttnUtil
except Exception as e:
    pass
try:
    sys.path.insert(0, testPath + 'lightning/unittests/')
    import LightningUtil
except Exception as e:
    pass


class UnitTestUtil:
    """
    SensorData supports wedge, wedge100, cmm, galaxy100, fbttn, and fbtp
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
        elif platformType == 'fbttn':
            return FbttnUtil.FbttnUtil()
        elif platformType == 'lightning':
            return LightningUtil.LightningUtil()
        else:
            raise Exception("Unsupported Platform")

    def JSONparser(self, file):
        """
        Get a json file and create a dictionary
        """
        with open(file) as data_file:
            data = json.load(data_file)
        return data

    def Argparser(self, names, platformTypes, helps):
        parser = argparse.ArgumentParser()
        for i in range(len(names)):
            if platformTypes[i] is None:
                parser.add_argument(names[i], help=helps[i])
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
        if Multiple:
            child = pexpect.spawn('scp -r ' + path + ' ' + dest)
        else:
            child = pexpect.spawn('scp ' + path + ' ' + dest)
        auth = child.expect(['password:', pexpect.EOF])
        if not auth:
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
