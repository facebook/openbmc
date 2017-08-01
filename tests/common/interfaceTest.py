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
    f = subprocess.Popen(['ip', 'addr', 'show', ifname],
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    out, err = f.communicate()
    if (len(out) == 0 or len(err) != 0):
        raise Exception("Device " + ifname + " does not exist [FAILED]")
    ipv4 = out.split("inet ")[1].split("/")[0]
    return ipv4


def get_ip6_address(ifname):
    """
    Get IPv6 address of a given interface
    """
    f = subprocess.Popen(['ip', 'addr', 'show', ifname],
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    out, err = f.communicate()
    if (len(out) == 0 or len(err) != 0):
        raise Exception("Device " + ifname + " does not exist [FAILED]")
    ipv6 = out.split("inet6 ")[1].split("/")[0]
    return ipv6


def pingConnection(ifname, ver):
    """
    Test connection of a given interface and version of IP address
    """
    if int(ver) == 4:
        logger.debug("getting ip address for " + str(ifname))
        ip = get_ip4_address(ifname)
        cmd = "ping -c 1 -q -w 1 " + ip
        logger.debug("executing: " + str(cmd))
        f = subprocess.Popen(cmd, shell=True,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        err = f.communicate()[1]
        if len(err) > 0:
            raise Exception(err)
    elif int(ver) == 6:
        logger.debug("getting ip address for " + str(ifname))
        ip = get_ip6_address(ifname)
        cmd = "ping -c 1 -q -w 1 " + ip
        logger.debug("executing: " + str(cmd))
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
            print('No IPv' + ver + ' address for ' + ifname +
                  ' interface [FAILED]')
        else:
            print("interface Test [FAILED]")
            print("Error: " + str(e))
        sys.exit(1)
