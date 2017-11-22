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

# Intended to compatible with both Python 2.7 and Python 3.x.
from __future__ import absolute_import
from __future__ import print_function
from __future__ import unicode_literals
from __future__ import division

from virtualcat import VirtualCat

import binascii
import hashlib
import json
import logging
import struct

# The typing module isn't installed on BMCs as of 2017-06-18 but it's only
# needed when running mypy on a developer's machine.
try:
    from typing import Any, List, Optional
except ImportError:
    pass


class InvalidPartitionException(Exception):
    pass


class Partition(object):
    '''
    Base partition class. We expect subclasses to implement specialized
    versions of the initialize, update, and finalize methods. Currently used
    for the "data0" partition because we let its contents vary arbitrarily
    (although we would ideally check its jffs2 filesystem integrity).
    '''
    def initialize(self):
        # type: () -> None
        self.length = 0

    def update(self, data):
        # type: (bytes) -> None
        self.length += len(data)

    def finalize(self):
        # type: () -> None
        if self.length != self.partition_size:
            self.valid = False
            self.logger.warning('Read {} of {} expected bytes from {}.'.format(
                self.length, self.partition_size, self
            ))
        else:
            self.logger.info('{} readable.'.format(self))

    def enforce_types(self):
        if self.partition_size is None:
            raise InvalidPartitionException('Unknown partition size')
        if self.data_size is None:
            raise InvalidPartitionException('Unknown data size')

    # This initializer will be called by all subclasses, so all subclass
    # overridden logic and types must be encapsulated in the above checksum
    # methods.
    def __init__(self, partition_size, partition_offset, name, images, logger):
        # type: (Optional[int], int, str, VirtualCat, logging.Logger) -> None
        self.valid = True
        self.partition_size = partition_size  # type: Optional[int]
        self.partition_offset = partition_offset
        self.name = name
        self.data_size = partition_size
        self.logger = logger
        self.initialize()
        # Unsuccessful attempt to make flake8 stop complaining about
        # int + Optional[int]
        self.enforce_types()
        images.read_with_callback(self.data_size, self.update)
        self.finalize()

    def __str__(self):
        # type: () -> str
        if self.partition_size is None:
            size = '0x???????'
        else:
            size = '0x{:07x}'.format(self.partition_size)
        return '{}@0x{:07x}({})'.format(
            size, self.partition_offset, self.name
        )

    def end(self):
        # type: () -> int
        self.enforce_types()
        return self.partition_offset + self.partition_size


class ExternalChecksumPartition(Partition):
    '''
    Calculates an md5sum for the whole partition and considers it valid if the
    computed checksum matches any of the reference checksums. Currently used
    for the "u-boot" partition because it does not yet contain any built-in
    checksum.
    '''
    UBootMagics = [0x130000ea, 0xb80000ea, 0xbe0000ea]

    def initialize(self):
        # type: () -> None
        self.md5sum = hashlib.md5()

    def update(self, data):
        # type: (bytes) -> None
        self.md5sum.update(data)

    def finalize(self):
        # type: () -> None
        if self.md5sum.hexdigest() not in self.checksums:
            self.valid = False
            self.logger.error(
                '{} md5sum {} not in {}.'.format(
                    self, self.md5sum.hexdigest(), self.checksums
                )
            )
        else:
            self.logger.info('{} has known good md5sum.'.format(self))

    def __init__(self, size, offset, name, images, checksums, logger):
        # type: (int, int, str, VirtualCat, List[str], logging.Logger) -> None
        self.checksums = checksums
        Partition.__init__(self, size, offset, name, images, logger)


class EnvironmentPartition(Partition):
    '''
    Base class for CRC32 checksummed partitions. The header class variables
    (overridden by subclasses) describe a 32 bit checksum for the partition of
    externally described size. Used for the "env" partition.
    '''
    header_format = b'<I'
    header_size = struct.calcsize(header_format)
    header_fields = ['data_crc32']

    def initialize(self):
        # type: () -> None
        self.data_crc32 = 0
        if self.partition_size is None:
            raise InvalidPartitionException('Unknown partition size')
        self.data_size = self.partition_size - self.header_size

    def update(self, data):
        # type: (bytes) -> None
        self.data_crc32 = binascii.crc32(data, self.data_crc32) & 0xffffffff

    def finalize(self):
        # type: () -> None
        if self.data_crc32 != self.parsed_header['data_crc32']:
            self.valid = False
            if (
                self.parsed_header['data_crc32'] == 0 and
                self.data_crc32 == 0x8b2a7ae8
            ):
                self.logger.info('{} zeroed out.'.format(self))
            else:
                message = '{} data crc32 0x{:08x} != expected 0x{:08x}.'
                self.logger.error(message.format(
                        self, self.data_crc32, self.parsed_header['data_crc32']
                ))
        else:
            self.logger.info('{} has valid data crc32.'.format(self))

    def __init__(self, size, offset, name, images, logger):
        # type: (Optional[int], int, str, VirtualCat, logging.Logger) -> None
        raw_header = images.verified_read(self.header_size)
        self.parsed_header = dict(zip(self.header_fields, struct.unpack(
            self.header_format, raw_header
        )))
        # TODO use enum names, hex, omit trailing nulls in strings, etc.
        logger.info(json.dumps(self.parsed_header))
        Partition.__init__(self, size, offset, name, images, logger)


class LegacyUBootPartition(EnvironmentPartition):
    '''
    Partition class which parses a (legacy) U-Boot header. The data region
    begins at the end of the header and ends where the size field in the header
    indicates. Used for "kernel" and "rootfs" partitions. Calls
    VirtualCat.seek_within_current_file() for any unused space between the end
    of the data region and the end of the partition. Takes a list (ordered
    smallest to largest) of potential partition sizes as a parameter and
    populates the corresponding property with the first one that the data fits
    inside.
    '''
    header_format = b'>7I4B32s'
    header_size = struct.calcsize(header_format)
    header_fields = ['magic', 'header_crc32', 'creation_time', 'data_size',
                     'load_address', 'entry_address', 'data_crc32', 'os',
                     'cpu_architecture', 'image_type', 'compression_type',
                     'name']
    magic = 0x27051956

    def initialize(self):
        # type: () -> None
        if self.parsed_header['magic'] != LegacyUBootPartition.magic:
            self.valid = False
            message = '{} header magic 0x{:08x} != expected 0x{:08x}.'
            self.logger.error(message.format(
                    self.name,
                    self.parsed_header['magic'],
                    LegacyUBootPartition.magic
            ))
        values = []  # type: List[Any]
        for field in self.header_fields:
            if field == 'header_crc32':
                values.append(0)
            else:
                values.append(self.parsed_header[field])
        modified_header = struct.pack(self.header_format, *values)
        header_crc32 = binascii.crc32(modified_header, 0) & 0xffffffff
        if header_crc32 != self.parsed_header['header_crc32']:
            self.valid = False
            self.logger.error(
                '{} header crc32 0x{:08x} != expected 0x{:08x}.'.format(
                    self, header_crc32, self.parsed_header['header_crc32']
                )
            )
        self.data_crc32 = 0
        self.data_size = self.parsed_header['data_size']
        self.total_size = self.header_size + self.data_size
        for size in self.sizes:
            if size >= self.total_size:
                self.partition_size = size
                break
        else:
            self.valid = False
            self.logger.warning(
                'Size 0x{:x} of {} greater than possibilities [{}].'.format(
                    self.total_size,
                    self,
                    ', '.join(['0x{:x}'.format(s) for s in self.sizes])
                )
            )

    def __init__(self, sizes, offset, name, images, logger):
        # type: (List[int], int, str, VirtualCat, logging.Logger) -> None
        self.sizes = sizes
        EnvironmentPartition.__init__(self, None, offset, name, images, logger)
        images.seek_within_current_file(self.partition_size - self.total_size)


class DeviceTreePartition(Partition):
    '''
    Partition class which parses a device tree / flattened image tree header.
    Both the beginning and end of the data region are determined from fields in
    the header. Used for "kernel" and "rootfs" partitions.
    '''
    def __init__(self, size, offset, name, images, logger):
        # type: (int, int, str, VirtualCat, logging.Logger) -> None
        Partition.__init__(self, size, offset, name, images, logger)
