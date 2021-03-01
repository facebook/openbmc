#!/usr/bin/env python3
#
# Copyright 2019-present Facebook. All Rights Reserved.
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

import argparse
import json
import ssl
import sys


try:
    from urllib.error import HTTPError
    from urllib.request import urlopen
except BaseException:
    from urllib.request import urlopen
    from urllib2 import HTTPError


class Log_Simple:
    def __init__(self, verbose=False):
        self._verbose = verbose
        self.fp = sys.stdout

    def verbose(self, message):
        if self._verbose:
            print(message, file=self.fp)

    def info(self, message):
        print(message, file=self.fp)

    def error(self, message):
        print(message, file=self.fp)


class OpenBMCApiCrawler(object):
    def __init__(self, url=None, encrypt=None, cert=None, key=None, logger=None):
        self.logger = logger
        self.endpoints = {}
        self.encrypt = encrypt
        self.ctx = None
        if not url:
            self.base_url = (
                "http://localhost:8080" if not encrypt else "https://localhost:8443"
            )
        else:
            self.base_url = url
        self.logger.verbose("check api endpoint with URL: {}".format(self.base_url))
        if encrypt:
            self.ctx = ssl.SSLContext()
            if cert or key:
                self.ctx.load_cert_chain(cert, key)
                self.logger.verbose(
                    "load client crt/key from: {} and {}".format(cert, key)
                )

    def open(self, url=None):
        """[summary] oepn url and return its Resource

        Args:
            url (str, optional): uri (ex. /api/sensors). Defaults to None.

        Returns:
            list: a list of available resources
        """
        if not url:
            return
        try:
            resp = urlopen(self.base_url + url, context=self.ctx)
        except HTTPError as e:
            self.logger.error("fail to open endpoint {} due to: {}".format(url, str(e)))
            return
        data = json.loads(resp.read().decode("utf-8"))
        info = data["Information"]
        if info and isinstance(info, dict):
            self.endpoints[url] = [title for title, _ in info.items()]
        elif info and isinstance(info, list):
            self.endpoints[url] = [_info.split(":")[0] for _info in info]
        return data["Resources"]

    def crawler(self, url=None):
        if not url:
            return
        _r = self.open(url)
        if not _r:
            return
        else:
            for nxt_resource in _r:
                nxt_url = url + "/" + nxt_resource
                self.logger.verbose("loading {}".format(nxt_url))
                self.crawler(nxt_url)

    def dump_api_info_py_format(self, fp):
        import pprint

        self.logger.verbose("output in py format")
        fp.write("api_info_list = ")
        fp.write(pprint.pformat(self.endpoints, indent=4))
        fp.write("\n")

    def dump_api_info_json_format(self, fp):
        import json

        self.logger.verbose("output in json format")
        fp.write(json.dumps(self.endpoints, sort_keys=True, indent=4))
        fp.write("\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        default=False,
        help="increase output verbosity",
    )
    parser.add_argument(
        "-e",
        "--encrypt",
        action="store_true",
        default=False,
        help="enable SSL",
    )
    parser.add_argument(
        "-c",
        "--cert",
        action="store",
        default=None,
        help="specify cert path",
    )
    parser.add_argument(
        "-k",
        "--key",
        action="store",
        default=None,
        help="specify key path",
    )
    parser.add_argument(
        "-u",
        "--url",
        action="store",
        default=None,
        help="specify url path (ex. https://1.2.3.4:8443)",
    )
    parser.add_argument(
        "-j",
        "--json-format",
        action="store_true",
        default=False,
        help="generate gpio info in json format",
    )
    parser.add_argument(
        "-a",
        "--api",
        action="store",
        default="/api",
        help="specify the api endpoint, default: /api",
    )
    parser.add_argument("outfile", nargs="?", default="-")
    args = parser.parse_args()

    if (args.cert and args.key is None) or (args.key and args.cert is None):
        parser.error("cert/key (-c, -k) must both present")

    logger = Log_Simple(verbose=args.verbose)

    if args.outfile == "-":
        logger.fp = sys.stderr
        logger.verbose("writing rest api info to %s.." % "/dev/stdout")
        outfile = sys.stdout
    else:
        logger.verbose("writing rest api info to %s.." % args.outfile)
        outfile = open(args.outfile, "w")

    endpts_crawler = OpenBMCApiCrawler(
        encrypt=args.encrypt, url=args.url, cert=args.cert, key=args.key, logger=logger
    )
    endpts_crawler.crawler(args.api)
    if args.json_format:
        endpts_crawler.dump_api_info_json_format(outfile)
    else:
        endpts_crawler.dump_api_info_py_format(outfile)

    print("Complete")
    sys.exit(0)
