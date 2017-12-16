from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import sys
import subprocess
import unitTestUtil
import logging
import socket

CURL_CMD = "curl -g http://{}:8080{} | python -m json.tool"
CURL_CMD6 = "curl -g http://[{}]:8080{} | python -m json.tool"


def check_ip6(ip_addr):
    '''
    TODO: Move to Py3 in flitr.tw and use inbuilt ipaddress module
    '''
    try:
        socket.inet_pton(socket.AF_INET6, ip_addr)
    except socket.error:
        return False
    else:
        return True


def restapiTest(data, host, logger):
    curl_cmd = ""
    try:
        if check_ip6(host):
            curl_cmd = CURL_CMD6
        else:
            curl_cmd = CURL_CMD
    except Exception:
        print("Provided host information invalid: hostname/ipv4/ipv6 address")
        curl_cmd = CURL_CMD
    for endpoint in data['endpoints']:
        cmd = curl_cmd.format(host, endpoint)
        logger.debug("Executing cmd={}".format(cmd))
        try:
            f = subprocess.Popen(cmd,
                                 shell=True,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
            result, err = f.communicate()
            for item in data['endpoints'][endpoint]:
                item = item.encode()
                if item not in result:
                    print("Expected {} in response={}", format(item, result))
                    print("Rest-api test [FAILED]")
                    sys.exit(1)
        except Exception:
            print("Rest-api test [FAILED]")
            sys.exit(1)
    print("Rest-api test [PASSED]")
    sys.exit(0)


if __name__ == "__main__":
    """
    Input to this file should look like the following:
    python restapiTest.py rest_endpoint.json
    """
    util = unitTestUtil.UnitTestUtil()
    logger = util.logger(logging.WARN)
    ifv6 = False
    try:
        data = {}
        args = util.Argparser(['bmc_ip', 'json', '--verbose'],
                              [str, str, None],
                              ['A host name or IP address for BMC',
                               'json file',
                               'Output all steps from test with mode options: \
                                    DEBUG, INFO, WARNING, ERROR'])
        if args.verbose is not None:
            logger = util.logger(args.verbose)
        host = args.bmc_ip
        data = util.JSONparser(args.json)
        restapiTest(data, host, logger)
    except Exception as e:
        print("Rest-api test [FAILED]")
        print("Error code returned: {}".format(e))
    sys.exit(1)
