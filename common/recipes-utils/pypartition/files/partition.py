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

from datetime import date

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
        return self.partition_offset + self.partition_size


class ExternalChecksumPartition(Partition):
    '''
    Calculates an md5sum for the whole partition and considers it valid if the
    computed checksum matches any of the reference checksums. Currently used
    for the "u-boot" partition because it does not yet contain any built-in
    checksum.
    '''
    UBootMagics = [0x130000ea, 0xb80000ea, 0xbe0000ea, 0xf0000ea]

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
        self.info_strings = []
        for (key, value) in self.parsed_header.items():
            if (
                key == 'magic' or key.endswith('_address') or
                key.endswith('_crc32')
            ):
                value_string = '0x{:08x}'.format(value)
            # Null-delimited bytes to space-delimited str, merging delimiters
            elif isinstance(value, bytes):
                value_string = ' '.join(
                    [b.decode() for b in value.split(b'\x00') if b]
                )
            elif key.endswith('_time'):
                value_string = date.fromtimestamp(value).isoformat()
            else:
                value_string = str(value)
            self.info_strings.append('{}: {}'.format(key, value_string))
        # TODO use enum names
        logger.info(', '.join(self.info_strings))
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
    header_fields = ['magic', 'header_crc32', 'creation_time', 'data_size',
                     'load_address', 'entry_address', 'data_crc32', 'os',
                     'cpu_architecture', 'image_type', 'compression_type',
                     'name']
    header_format = b'>7I4B32s'
    header_size = struct.calcsize(header_format)
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

    def __init__(self, sizes, offset, name, images, logger, grow_until=None):
        # type: (List[int], int, str, VirtualCat, logging.Logger, Optional[int]) -> None
        self.sizes = sizes
        EnvironmentPartition.__init__(self, None, offset, name, images, logger)

        images.seek_within_current_file(self.partition_size - self.total_size)

        if grow_until is None:
            return

        sizes_to_check = self.sizes[self.sizes.index(self.partition_size):]
        for size in sizes_to_check:
            images.seek_within_current_file(size - self.partition_size)
            self.partition_size = size
            if images.peek() == grow_until:
                return

        self.valid = False
        sizes_to_check_string = ' or '.join(
            ['0x{:x}'.format(s) for s in sizes_to_check]
        )
        self.logger.warning(
            'Could not find magic 0x{:x} after {}.'.format(
                grow_until, sizes_to_check_string
            )
        )


class DeviceTreePartition(Partition):
    '''
    Partition class which parses a device tree / flattened image tree header.
    Both the beginning and end of the data region are determined from fields in
    the header. Used for "kernel" and "rootfs" partitions.
    '''
    header_fields = [
        'magic', 'total_size', 'structure_block_offset', 'strings_block_offset',
        'memory_reservation_map_offset', 'version', 'last_compatible_version',
        'boot_cpu', 'strings_block_size', 'structure_block_size'
    ]
    header_format = b'>%dI' % len(header_fields)
    header_size = struct.calcsize(header_format)
    magic = 0xd00dfeed
    FDT_BEGIN_NODE = 1
    FDT_END_NODE = 2
    FDT_PROP = 3
    FDT_NOP = 4
    FDT_END = 9
    recognized_hexadecimals = [
        b'#address-cells', b'data-size', b'data-position', b'load',
        b'entry', b'hashed-strings', b'value',
    ]
    recognized_strings = [
        b'algo', b'arch', b'compression', b'default', b'description',
        b'firmware', b'hashed-nodes', b'kernel', b'key-name-hint', b'os',
        b'ramdisk', b'sign-images', b'signer-name', b'signer-version',
        b'type',
    ]
    unrecognized_string_message = \
        'Attempting to parse unrecognized {}-byte property "{}" as string.'

    @staticmethod
    def align(images):
        # type: (VirtualCat) -> List[Any]
        while images.open_file.tell() % 4 != 0:
            images.verified_read(1)

    @staticmethod
    def next_data(images, length, data_type):
        # type: (VirtualCat, int, str) -> List[Any]
        fmt = b'>%d%s' % (length // struct.calcsize(data_type), data_type)
        assert(length == struct.calcsize(fmt))
        start = images.open_file.tell()
        data = struct.unpack(fmt, images.verified_read(length))
        end = images.open_file.tell()
        DeviceTreePartition.align(images)
        return list(data)

    @staticmethod
    def next_datum(images, length, datum_type):
        data = DeviceTreePartition.next_data(images, length, datum_type)
        assert(len(data) == 1)
        return data[0]

    @staticmethod
    def dict_from_node(images, strings, logger):
        # type: (VirtualCat, bytes, logging.Logger) -> Dict[bytes, Any]]
        node_name = ''
        tree = {}
        while True:
            token = DeviceTreePartition.next_datum(images, 4, b'I')

            if token == DeviceTreePartition.FDT_BEGIN_NODE:
                node_name = DeviceTreePartition.next_datum(images, 4, b's')
                while node_name.find(b'\x00') < 0:
                    node_name += DeviceTreePartition.next_datum(images, 4, b's')
                node_name = node_name.rstrip(b'\x00')
                tree[node_name] = DeviceTreePartition.dict_from_node(
                    images, strings, logger
                )
            elif token == DeviceTreePartition.FDT_END_NODE:
                return tree
            elif token == DeviceTreePartition.FDT_PROP:
                property_name, value = DeviceTreePartition.property_name_value(
                    images, strings, logger
                )
                tree[property_name] = value
            elif token == DeviceTreePartition.FDT_NOP:
                pass
            elif token == DeviceTreePartition.FDT_END:
                raise InvalidPartitionException('FDT_END before FDT_END_NODE')
            else:
                raise InvalidPartitionException('Unsupported token {}'.format(
                    token
                    ))

    @staticmethod
    def property_name_value(images, strings, logger):
        # type: (VirtualCat, bytes, logging.Logger) -> (bytes, bytes)
        length = DeviceTreePartition.next_datum(images, 4, b'I')
        offset = DeviceTreePartition.next_datum(images, 4, b'I')
        name = strings[offset:strings.index(b'\x00', offset)]
        if name == b'timestamp':
            value = date.fromtimestamp(DeviceTreePartition.next_datum(images, length, b'I')).isoformat()
        elif name == b'data':
            sha256sum = hashlib.sha256()
            images.read_with_callback(length, sha256sum.update)
            value = sha256sum.hexdigest()
            DeviceTreePartition.align(images)
        elif name in DeviceTreePartition.recognized_hexadecimals:
            separator = b'' if name == b'value' else b' '
            value = separator.join(
                [b'%08x' % datum for datum in DeviceTreePartition.next_data(
                    images, length, b'I'
                )]
            )
        else:
            if name not in DeviceTreePartition.recognized_strings:
                logger.warning(
                    DeviceTreePartition.unrecognized_string_message.format(
                        length, name.decode()
                    )
                )
            value = DeviceTreePartition.next_datum(images, length, b's').rstrip(b'\x00')
        return (name, value)

    def __init__(self, sizes, offset, name, images, logger):
        # type: (int, List<int>, str, VirtualCat, logging.Logger) -> None
        self.name = name
        self.partition_offset = offset
        self.valid = True

        raw_header = images.verified_read(self.header_size)
        parsed_header = dict(zip(self.header_fields, struct.unpack(
            self.header_format, raw_header
        )))

        info_strings = []
        for (key, value) in parsed_header.items():
            if (
                key == 'magic' or key.endswith('_size') or
                key.endswith('_offset')
            ):
                value_string = '0x{:08x}'.format(value)
            else:
                value_string = str(value)
            info_strings.append('{}: {}'.format(key, value_string))
        logger.info(', '.join(info_strings))

        if parsed_header['magic'] != DeviceTreePartition.magic:
            self.valid = False
            message = '{} header magic 0x{:08x} != expected 0x{:08x}.'
            logger.error(message.format(
                name, parsed_header['magic'], DeviceTreePartition.magic
            ))

        if parsed_header['total_size'] > sizes[-1]:
            self.valid = False
            message = '{} size 0x{:08x} > expected 0x{:08x}.'
            logger.error(message.format(
                name, parsed_header['total_size'], sizes[-1]
            ))

        # Skip over memory reservation and structure blocks to strings block.
        images.seek_within_current_file(
            parsed_header['strings_block_offset'] - self.header_size
        )
        strings = images.verified_read(parsed_header['strings_block_size'])

        # Now go back to structure block.
        distance = (
            parsed_header['structure_block_offset'] -
            parsed_header['strings_block_offset'] -
            parsed_header['strings_block_size']
        )
        images.seek_within_current_file(distance)

        null = b'\x00\x00\x00\x00'
        if (
            DeviceTreePartition.next_datum(images, 4, b'I') != 1 or
            DeviceTreePartition.next_datum(images, 4, b's') != null
        ):
            raise InvalidPartitionException(
                'Problem with first FDT_BEGIN_NODE.'
            )

        tree = DeviceTreePartition.dict_from_node(images, strings, logger)
        logger.info(tree)

        if DeviceTreePartition.next_datum(images, 4, b'I') != 9:
            raise InvalidPartitionException('Problem with FDT_END.')

        for name, properties in tree[b'images'].items():
            assert(properties[b'hash@1'][b'algo'] == b'sha256')
            expected = properties[b'hash@1'][b'value'].decode()
            if b'data-size' in properties and b'data-position' in properties:
                # The data is at a fixed offset, seek to it to compute
                # the checksum and seek back to the current location.
                data_pos = int(properties[b'data-position'], 16)
                data_len = int(properties[b'data-size'], 16)
                curr_tell = images.open_file.tell()
                images.open_file.seek(self.partition_offset + data_pos)
                sha256sum = hashlib.sha256()
                images.read_with_callback(data_len, sha256sum.update)
                properties[b'data'] = sha256sum.hexdigest()
                images.open_file.seek(curr_tell)
            if properties[b'data'] != expected:
                self.valid = False
                logger.error('{} {} {} != {}.'.format(
                    name, properties[b'hash@1'][b'algo'], properties[b'data'],
                    expected
                ))
            else:
                logger.info('{} checksums match.'.format(name))

        used_size = images.open_file.tell() - self.partition_offset
        for sz in sizes:
            if (used_size < sz):
                self.partition_size = sz;
                break

        # Go to end of partition.
        images.seek_within_current_file(
                self.partition_size -
                parsed_header['structure_block_offset'] -
                parsed_header['structure_block_size']
        )

        logger.info('{} has valid checksums.'.format(self))
