#!/usr/bin/env python3
# Intended to compatible with both Python 2.7 and Python 3.x.

# Copyright 2017-present Facebook. All Rights Reserved.
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

from __future__ import absolute_import, division, print_function, unicode_literals

import json
import os
import socket
import sys
import textwrap

import system
import virtualcat


try:
    import http.server as http_server
    import socketserver
except ImportError:
    import SimpleHTTPServer as http_server
    import SocketServer as socketserver


class V6TCPServer(socketserver.TCPServer):
    address_family = socket.AF_INET6


def check_image(logger):
    # type: (logging.Logger) -> None
    if system.is_openbmc():
        logger.warning("Running {} from an OpenBMC is untested.".format(sys.argv[0]))

    description = textwrap.dedent(
        """\
        Validate image file partition checksums and optionally serve the file
        via HTTP for OpenBMCs to fetch."""
    )
    (checksums, args) = system.get_checksums_args(description)

    if not args.image:
        logger.error("No image specified")
        sys.exit(1)

    if args.append_new_checksums:
        list(checksums).append("PLACEHOLDER")

    partitions = system.get_valid_partitions(
        [virtualcat.ImageFile(args.image)], checksums, logger
    )

    if args.append_new_checksums:
        checksums = []
        [checksums.extend(p.checksums) for p in partitions if hasattr(p, "checksums")]
        logger.info(
            "Writing appended checksum list to {}.".format(
                args.append_new_checksums.name
            )
        )
        json.dump({checksum: "" for checksum in checksums}, args.append_new_checksums)

    if args.serve:
        directory = os.path.dirname(args.image)
        logger.info("Changing directory to {}.".format(directory))
        os.chdir(directory)
        Handler = http_server.SimpleHTTPRequestHandler
        logger.info("Serving HTTP on port {}".format(args.port))
        httpd = V6TCPServer(("::", args.port), Handler)
        httpd.serve_forever()

    sys.exit(0)


if __name__ == "__main__":
    logger = system.get_logger()
    try:
        check_image(logger)
    except Exception:
        logger.exception("Unhandled exception raised.")
