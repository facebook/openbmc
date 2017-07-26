#!/usr/bin/env python3
#
# Copyright 2015-present Facebook. All Rights Reserved.
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

import urllib.request, urllib.error, urllib.parse, urllib.request, urllib.parse, urllib.error
import logging
import os
import subprocess
import time
import argparse
import re

SPATULA_FILE = '/etc/spatula/spatula'
LLDP_UTIL = '/usr/bin/lldp-util'
LOG_FORMAT = '[%(levelname)s] %(asctime)s: %(message)s'
DATE_FORMAT = '%Y-%m-%d %H:%M:%S'
DEFAULT_SLEEP = 900 # default sleep 15 mins
DEFAULT_PORT = 8087
DEFAULT_INTERFACE = 'usb0'

class SpatulaWrapper(object):
    def __init__(self, address='fe80::2', interface='usb0',
            port=8087, tls=False, lldp=False):
        self.interface = interface
        self.lldp = lldp
        self.port = port
        self.proto = 'http'
        if tls:
            self.proto = 'https'
        # not used if auto-discovering
        self.url = '{proto}://{address}%{iface}:{port}'.format(
                proto = self.proto,
                address = address,
                iface = self.interface,
                port = self.port)

    def _getLldpMessage(self):
        try:
            if os.path.exists(LLDP_UTIL):
                # request a single packet and time out in 30 seconds
                lldp = subprocess.Popen([LLDP_UTIL, '-n', '4', '-t', '30'],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE)
                found = []
                for line in lldp.stdout:
                    # example (one line):
                    # LLDP: local_port=eth0 \
                    # remote_system=rsw1bz.19.snc1.facebook.com \
                    # remote_port=00:90:fb:52:1a:49
                    if line.startswith('LLDP: '):
                        matches = re.findall(r'\w+=\S+', line.replace('LLDP: ', ''))
                        matches = [m.split('=', 1) for m in matches]
                        found.append(dict(matches))
                if not found:
                    raise Exception('timed out waiting for LLDP packet')
                # dedupe before returning since we may get multiple packets
                # from the same device
                return [dict(t) for t in set([tuple(d.items()) for d in found])]
            else:
                raise Exception('missing lldp-util: {}'.format(LLDP_UTIL))
        except Exception as err:
            raise Exception('failed discovering management system via LLDP: {}'.format(err))

    def _getSpatula(self, endpoint='/spatula'):
        '''
        Get the executable spatula script from the host.
        '''
        urls = []
        urls.append('{}{}'.format(self.url, endpoint))
        if self.lldp:
            for lldp in self._getLldpMessage():
                urls.append('{proto}://{address}%{iface}:{port}{endpoint}'.format(
                    proto = self.proto,
                    address = lldp['remote_system'],
                    iface = lldp['local_port'],
                    port = self.port,
                    endpoint = endpoint))
        for url in urls:
            try:
                request = urllib.request.Request(url)
                response = urllib.request.urlopen(request)
                return response.read()
            except Exception as err:
                continue
        raise Exception('failed getting Spatula {}: {}'.format(urls, err))

    def _success(self, endpoint='/success'):
        '''
        Api to report the timestamp of a successful run
        '''
        query = urllib.parse.urlencode({'timestamp': time.time()})
        url = '{}{}?{}'.format(self.url, endpoint, query)
        try:
            request = urllib.request.Request(url)
            response = urllib.request.urlopen(request)
            return response.read()
        except Exception as err:
            raise Exception('failed report success {}: {}'.format(url, err))

    def _error(self, error, endpoint='/error'):
        '''
        Api to report the timestamp and the error when spatula fails
        '''
        query = urllib.parse.urlencode({'timestamp': time.time(), 'error': error})
        url = '{}{}?{}'.format(self.url, endpoint, query)
        try:
            request = urllib.request.Request(url)
            response = urllib.request.urlopen(request)
            return response.read()
        except Exception as err:
            raise Exception('failed report error [{}]: {}'.format(url, err))

    def _execute(self):
        try:
            # get the executable from host
            if not os.path.exists(os.path.dirname(SPATULA_FILE)):
                os.makedirs(os.path.dirname(SPATULA_FILE))
            with open(SPATULA_FILE, 'wb') as file:
                file.write(self._getSpatula())
                file.close()
            # set the permission
            os.chmod(SPATULA_FILE, 0o755)
            # run the executable file
            spatula = subprocess.Popen([SPATULA_FILE],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE)
            out, err = spatula.communicate()
            if spatula.returncode != 0:
                raise Exception('spatula failed: {}'.format(err))
            self._success()
        except Exception as err:
            self._error(err)
            raise err

    def run(self, sleep):
        while True:
            try:
                self._execute()
            except Exception as err:
                logging.error(err)
            time.sleep(sleep)

if __name__ == '__main__':
    args = argparse.ArgumentParser()
    args.add_argument('-s', '--sleep', default=DEFAULT_SLEEP,
            help='Sleep time between spatula runs (default=%(default)s)')
    args.add_argument('--interface', '-i', default=DEFAULT_INTERFACE,
            help='Interface to use for management system')
    args.add_argument('--port', '-p', default=DEFAULT_PORT,
            help='Port to contact on management system')
    args.add_argument('--lldp', default=False, action="store_true",
            help='Automatically discover management system with LLDP')
    args.add_argument('--tls', default=False, action="store_true",
            help='Connect to bmc_proxy using TLS')
    params = args.parse_args()
    logging.basicConfig(format=LOG_FORMAT, datefmt=DATE_FORMAT)
    wrapper = SpatulaWrapper(interface=params.interface,
        tls=params.tls,
        port=params.port,
        lldp=params.lldp)
    wrapper.run(params.sleep)
