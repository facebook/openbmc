#!/usr/bin/env python
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

import urllib2, urllib
import logging
import os
import subprocess
import time
import argparse

SPATULA_FILE = '/etc/spatula/spatula'
LOG_FORMAT = '[%(levelname)s] %(asctime)s: %(message)s'
DATE_FORMAT = '%Y-%m-%d %H:%M:%S'
DEFAULT_SLEEP = 900 # default sleep 15 mins

class SpatulaWrapper(object):
    def __init__(self, address='fe80::2', interface='usb0',
                    port=8087, ssl=False):
        proto = 'http'
        if ssl:
            proto = 'https'
        self.url = '{proto}://{address}%{iface}:{port}'.format(
                proto = proto,
                address = address,
                iface = interface,
                port = port)

    def _getSpatula(self, endpoint='/spatula'):
        '''
        Get the executable spatula script from the host.
        '''
        url = '{}{}'.format(self.url, endpoint)
        try:
            request = urllib2.Request(url)
            response = urllib2.urlopen(request)
            return response.read()
        except Exception as err:
            raise Exception('failed getting Spatula {}: {}'.format(url, err))

    def _success(self, endpoint='/success'):
        '''
        Api to report the timestamp of a successful run
        '''
        query = urllib.urlencode({'timestamp': time.time()})
        url = '{}{}?{}'.format(self.url, endpoint, query)
        try:
            request = urllib2.Request(url)
            response = urllib2.urlopen(request)
            return response.read()
        except Exception as err:
            raise Exception('failed report success {}: {}'.format(url, err))

    def _error(self, error, endpoint='/error'):
        '''
        Api to report the timestamp and the error when spatula fails
        '''
        query = urllib.urlencode({'timestamp': time.time(), 'error': error})
        url = '{}{}?{}'.format(self.url, endpoint, query)
        try:
            request = urllib2.Request(url)
            response = urllib2.urlopen(request)
            return response.read()
        except Exception as err:
            raise Exception('failed report error [{}]: {}'.format(url, err))

    def _execute(self):
        try:
            # get the executable from host
            if not os.path.exists(os.path.dirname(SPATULA_FILE)):
                os.makedirs(os.path.dirname(SPATULA_FILE))
            with open(SPATULA_FILE, 'w+') as file:
                file.write(self._getSpatula())
                file.close()
            # set the permission
            os.chmod(SPATULA_FILE, 0755)
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
    params = args.parse_args()
    logging.basicConfig(format=LOG_FORMAT, datefmt=DATE_FORMAT)
    wrapper = SpatulaWrapper()
    wrapper.run(params.sleep)
