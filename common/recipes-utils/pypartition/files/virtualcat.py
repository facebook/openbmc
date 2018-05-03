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

from io import DEFAULT_BUFFER_SIZE

import os
import struct

try:
    from typing import Callable, List, Union
except Exception:
    pass


class ImageFile(object):
    '''
    Defines basic attributes for files and allows the MTD subclass to
    redefine them.
    '''
    def __init__(self, file_name):
        # type: (str) -> None
        self.file_name = file_name
        self.size = os.path.getsize(file_name)

    def __str__(self):
        # type: () -> str
        return '{} (0x{:x} bytes)'.format(self.file_name, self.size)


class MemoryTechnologyDevice(ImageFile):
    '''
    Defines MTD attributes so that they can be used mostly interchangeably with
    ImageFiles.
    '''
    def __init__(self, device, size, device_name, offset=0):
        # type: (str, int, str) -> None
        self.file_name = os.path.join('/dev', device)
        self.size = size
        self.device_name = device_name
        self.offset = offset

    def __str__(self):
        # type: () -> str
        return '{} ({}, 0x{:x} bytes)'.format(
            self.device_name, self.file_name, self.size
        )


try:
    ImageSourcesType = List[Union[ImageFile, MemoryTechnologyDevice]]
except Exception:
    pass


class VirtualCat(object):
    '''
    Abstracts the opening, reading, and closing of a sequence of ImageFiles or
    MemoryTechnologyDevices, almost as if they had been concatenated. Also
    provides a read_with_callback() method that reads the current open file in
    reasonably sized chunks. (One might expect a finite read() function to
    already exist somewhere within the Python standard libraries, but cov was
    unable to find one.) read() calls spanning multiple files are not
    supported. A special seek_within_current_file() method seeks forward to the
    end of the currently open file, or the specified amount, whichever comes
    first, closing the current file and opening the next if needed. peek()
    reads 4 bytes then seeks backwards that amount.
    '''
    def __init__(self, reference_image_list):
        # type: (ImageSourcesType) -> None
        # Copy so we can pop() without changing the caller's list
        self.images = []  # type: ImageSourcesType
        self.images.extend(reference_image_list)
        self.open_file = open(self.images[0].file_name, 'rb')

    def __enter__(self):
        # type: () -> VirtualCat
        return self

    def __exit__(self, exception_type, exception_value, exception_traceback):
        # type: (type, int, int) -> None
        self.open_file.close()

    def next_image_if_needed(self):
        # type: () -> None
        if self.open_file.tell() == self.images[0].size:
            self.open_file.close()
            self.images.pop(0)
            if len(self.images) > 0:
                self.open_file = open(self.images[0].file_name, 'rb')

    def verified_read(self, requested_size):
        # type: (int) -> bytes
        self.next_image_if_needed()
        data = self.open_file.read(requested_size)
        bytes_read = len(data)
        if bytes_read != requested_size:
            # Right now we do nothing before raising the exception, but it may
            # be possible to either seek() backwards, to make it appear as if
            # the read had never happened, or call next_image_if_needed(),
            # prior to raising the exception.
            raise IOError('read {} bytes but {} requested'.format(
                bytes_read, requested_size
            ))
        return data

    def peek(self):
        # type: () -> int
        word_format = b'>I'
        word_length = struct.calcsize(word_format)
        raw_word = self.verified_read(word_length)
        self.open_file.seek(word_length * -1, os.SEEK_CUR)
        return struct.unpack(word_format, raw_word)[0]

    def read_with_callback(self, size, callback):
        # type: (int, Callable[[bytes], None]) -> None
        foreword_size = 0
        if size > DEFAULT_BUFFER_SIZE:
            start_unaligned = self.open_file.tell() % DEFAULT_BUFFER_SIZE
            foreword_size = DEFAULT_BUFFER_SIZE - start_unaligned
            callback(self.verified_read(foreword_size))
        buffer_count = (size - foreword_size) // DEFAULT_BUFFER_SIZE
        for _ in range(buffer_count):
            callback(self.verified_read(DEFAULT_BUFFER_SIZE))
        afterword_size = \
            size - foreword_size - buffer_count * DEFAULT_BUFFER_SIZE
        if afterword_size > 0:
            callback(self.verified_read(afterword_size))

    def seek_within_current_file(self, amount):
        # type: (int) -> None
        if len(self.images) == 0:
            raise IOError('cannot skip beyond end of last file')
        position = min(self.open_file.tell() + amount, self.images[0].size)
        self.open_file.seek(position)
        self.next_image_if_needed()
