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

import logging
import os
import unittest
import virtualcat

try:
    from unittest.mock import MagicMock, call, mock_open, patch
except ImportError:
    from mock import MagicMock, call, mock_open, patch


class TestVirtualCat(unittest.TestCase):
    def setUp(self):
        logging.root.setLevel(logging.DEBUG)
        self.data = b'data'
        self.image = MagicMock()
        self.image.file_name = '/file/name'
        self.image.size = len(self.data)
        self.callback = MagicMock()

    @patch.object(virtualcat, 'open', new_callable=mock_open)
    def test_single_image_open_close(self, mocked_open):
        # __init__() and __enter__()
        with virtualcat.VirtualCat([self.image]) as vc:
            mocked_open.assert_called_once_with(self.image.file_name, 'rb')
            mock_file = vc.open_file
            mock_file.read.assert_not_called()
            mock_file.tell.assert_not_called()
            mock_file.seek.assert_not_called()
            mock_file.close.assert_not_called()
            self.callback.assert_not_called()
        # open_file is not None case of __exit__()
        self.assertEqual(mock_file.close.call_count, 1)

    @patch.object(virtualcat, 'open', new_callable=mock_open)
    def test_single_image_read_afterword_to_middle(self, mocked_open):
        return_value = mock_open(read_data=self.data[:2]).return_value
        mocked_open.return_value = return_value
        with virtualcat.VirtualCat([self.image]) as vc:
            mocked_open.assert_called_once_with(self.image.file_name, 'rb')
            mock_file = vc.open_file
            mock_file.tell.side_effect = [0]
            # tell() != size case of next_image_if_needed()
            vc.read_with_callback(2, self.callback)
            mock_file.read.assert_called_once_with(2)
            self.callback.assert_called_once_with(self.data[:2])
            self.assertEqual(mock_file.tell.call_count, 1)
            mock_file.seek.assert_not_called()
            mock_file.close.assert_not_called()
        # open_file is not None case of __exit__()
        self.assertEqual(mock_file.close.call_count, 1)

    @patch.object(virtualcat, 'open', new_callable=mock_open)
    def test_single_image_read_afterword(self, mocked_open):
        mocked_open.return_value = mock_open(read_data=self.data).return_value
        with virtualcat.VirtualCat([self.image]) as vc:
            mocked_open.assert_called_once_with(self.image.file_name, 'rb')
            mock_file = vc.open_file
            mock_file.tell.side_effect = [0, self.image.size]
            # foreword_size == 0, buffer_count == 0, afterword_size > 0
            # case of read_with_callback()
            # tell() == size case of next_image_if_needed()
            vc.read_with_callback(self.image.size, self.callback)
            mock_file.read.assert_called_once_with(self.image.size)
            self.callback.assert_called_once_with(self.data)
            self.assertNotEqual(mock_file.tell.call_count, 0)
            mock_file.seek.assert_not_called()
        # open_file is not None case of __exit__()
        self.assertEqual(mock_file.close.call_count, 1)

    @patch.object(virtualcat, 'open', new_callable=mock_open)
    def test_single_image_read_buffer(self, mocked_open):
        self.data = b'a' * DEFAULT_BUFFER_SIZE
        self.image.size = len(self.data)
        mocked_open.return_value = mock_open(read_data=self.data).return_value
        with virtualcat.VirtualCat([self.image]) as vc:
            mocked_open.assert_called_once_with(self.image.file_name, 'rb')
            mock_file = vc.open_file
            mock_file.tell.side_effect = [0, self.image.size]
            # foreword_size == 0, buffer_count > 1, afterword_size == 0
            # case of read_with_callback()
            vc.read_with_callback(self.image.size, self.callback)
            mock_file.read.assert_called_once_with(self.image.size)
            self.callback.assert_called_once_with(self.data)
            self.assertNotEqual(mock_file.tell.call_count, 0)
            mock_file.seek.assert_not_called()
        # open_file is not None case of __exit__()
        self.assertEqual(mock_file.close.call_count, 1)

    @patch.object(virtualcat, 'open', new_callable=mock_open)
    def test_single_image_read_buffer_afterword(self, mocked_open):
        self.image.size = DEFAULT_BUFFER_SIZE * 2 - 11
        with virtualcat.VirtualCat([self.image]) as vc:
            mocked_open.assert_called_once_with(self.image.file_name, 'rb')
            mock_file = vc.open_file
            tells = [
                0,
                0,
                DEFAULT_BUFFER_SIZE,
            ]
            reads = [
                b'a' * (DEFAULT_BUFFER_SIZE - tells[0]),
                b'b' * (DEFAULT_BUFFER_SIZE - 11),
            ]
            mock_file.read.side_effect = reads
            mock_file.tell.side_effect = tells
            # foreword_size > 0, buffer_count == 1, afterword_size == 0
            # case of read_with_callback()
            vc.read_with_callback(self.image.size - tells[0],
                                  self.callback)
            self.assertEqual(mock_file.read.mock_calls,
                             [call(len(data)) for data in reads])
            self.assertEqual(self.callback.mock_calls,
                             [call(data) for data in reads])
            self.assertEqual(mock_file.tell.call_count, len(tells))
            mock_file.seek.assert_not_called()
        # open_file is not None case of __exit__()
        self.assertEqual(mock_file.close.call_count, 1)

    # No foreword_size == 0, buffer_count > 0, afterword_size > 0 or
    # foreword_size > 0, buffer_count == 0, afterword_size == 0 cases because
    # foreword_size > 0 if and only if size > DEFAULT_BUFFER_SIZE.

    @patch.object(virtualcat, 'open', new_callable=mock_open)
    def test_single_image_read_foreword_afterword(self, mocked_open):
        more_than_half = DEFAULT_BUFFER_SIZE // 2 + 1
        self.image.size = more_than_half * 2
        foreword_size = DEFAULT_BUFFER_SIZE - more_than_half
        afterword_size = self.image.size - foreword_size
        # __init__() and __enter__()
        with virtualcat.VirtualCat([self.image]) as vc:
            mocked_open.assert_called_once_with(self.image.file_name, 'rb')
            mock_file = vc.open_file
            tells = [
                more_than_half,
                more_than_half,
                DEFAULT_BUFFER_SIZE,
            ]
            reads = [
                b'a' * foreword_size,
                b'b' * afterword_size
            ]
            mock_file.read.side_effect = reads
            mock_file.tell.side_effect = tells
            # foreword_size > 0, buffer_count == 0, afterword_size > 0 case
            # of read_with_callback()
            vc.read_with_callback(self.image.size, self.callback)
            self.assertEqual(mock_file.read.mock_calls,
                             [call(len(data)) for data in reads])
            self.assertEqual(self.callback.mock_calls,
                             [call(data) for data in reads])
            self.assertEqual(mock_file.tell.call_count, len(tells))
            mock_file.seek.assert_not_called()
        # self.open_file is not None case of __exit__
        self.assertEqual(mock_file.close.call_count, 1)

    @patch.object(virtualcat, 'open', new_callable=mock_open)
    def test_single_image_read_foreword_buffer(self, mocked_open):
        self.image.size = DEFAULT_BUFFER_SIZE * 2
        with virtualcat.VirtualCat([self.image]) as vc:
            mocked_open.assert_called_once_with(self.image.file_name, 'rb')
            mock_file = vc.open_file
            tells = [
                3,
                3,
                DEFAULT_BUFFER_SIZE,
            ]
            reads = [
                b'a' * (DEFAULT_BUFFER_SIZE - tells[0]),
                b'b' * DEFAULT_BUFFER_SIZE,
            ]
            mock_file.read.side_effect = reads
            mock_file.tell.side_effect = tells
            # foreword_size > 0, buffer_count == 1, afterword_size == 0
            # case of read_with_callback()
            vc.read_with_callback(self.image.size - tells[0],
                                  self.callback)
            self.assertEqual(mock_file.read.mock_calls,
                             [call(len(data)) for data in reads])
            self.assertEqual(self.callback.mock_calls,
                             [call(data) for data in reads])
            self.assertEqual(mock_file.tell.call_count, len(tells))
            mock_file.seek.assert_not_called()
        # open_file is not None case of __exit__()
        self.assertEqual(mock_file.close.call_count, 1)

    @patch.object(virtualcat, 'open', new_callable=mock_open)
    def test_single_image_read_foreword_buffers_afterword(self, mocked_open):
        self.image.size = DEFAULT_BUFFER_SIZE * 4 - 99
        with virtualcat.VirtualCat([self.image]) as vc:
            mocked_open.assert_called_once_with(self.image.file_name, 'rb')
            mock_file = vc.open_file
            tells = [
                99,
                99,
                DEFAULT_BUFFER_SIZE,
                DEFAULT_BUFFER_SIZE * 2,
                DEFAULT_BUFFER_SIZE * 3,
            ]
            reads = [
                b'a' * (DEFAULT_BUFFER_SIZE - tells[0]),
                b'b' * DEFAULT_BUFFER_SIZE,
                b'c' * DEFAULT_BUFFER_SIZE,
                b'd' * (DEFAULT_BUFFER_SIZE - 99)
            ]
            mock_file.read.side_effect = reads
            mock_file.tell.side_effect = tells
            # foreword_size > 0, buffer_count == 1, afterword_size > 0
            # case of read_with_callback()
            vc.read_with_callback(self.image.size - tells[0],
                                  self.callback)
            self.assertEqual(mock_file.read.mock_calls,
                             [call(len(data)) for data in reads])
            self.assertEqual(self.callback.mock_calls,
                             [call(data) for data in reads])
            self.assertEqual(mock_file.tell.call_count, len(tells))
            mock_file.seek.assert_not_called()
        # open_file is not None case of __exit__()
        self.assertEqual(mock_file.close.call_count, 1)

    @patch.object(virtualcat, 'open', new_callable=mock_open)
    def test_single_image_small_read_past_end(self, mocked_open):
        # __init__() and __enter__()
        with virtualcat.VirtualCat([self.image]) as vc:
            mocked_open.assert_called_once_with(self.image.file_name, 'rb')
            mock_file = vc.open_file
            mock_file.tell.side_effect = [0, self.image.size]
            # size < DEFAULT_BUFFER_SIZE case of read_with_callback()
            with self.assertRaises(IOError):
                vc.read_with_callback(self.image.size + 1, self.callback)
            mock_file.read.assert_called_once_with(self.image.size + 1)
            self.callback.assert_not_called()
        # To accomodate the possibility of cleanup like seek()ing backwards or
        # calling next_image_if_needed() being added prior to the exception
        # being raised, don't check whether tell() or seek() were called or
        # when exactly open_file (mock_file) is closed.
        self.assertEqual(mock_file.close.call_count, 1)

    @patch.object(virtualcat, 'open', new_callable=mock_open)
    def test_single_image_skip_to_middle(self, mocked_open):
        # __init__() and __enter__()
        with virtualcat.VirtualCat([self.image]) as vc:
            mocked_open.assert_called_once_with(self.image.file_name, 'rb')
            mock_file = vc.open_file
            tells = [0, 2]
            mock_file.tell.side_effect = tells
            # open_file is not None, position == tell() + amount case of
            # seek_within_current_file; tell() != size case of
            # next_image_if_needed()
            vc.seek_within_current_file(2)
            mock_file.read.assert_not_called()
            self.callback.assert_not_called()
            self.assertEqual(mock_file.tell.call_count, len(tells))
            mock_file.seek.assert_called_once_with(2)
            mock_file.close.assert_not_called()
        # open_file is not None case of __exit__()
        self.assertEqual(vc.open_file.close.call_count, 1)

    @patch.object(virtualcat, 'open', new_callable=mock_open)
    def test_single_image_skip_to_end_then_beyond(self, mocked_open):
        # __init__() and __enter__()
        with virtualcat.VirtualCat([self.image]) as vc:
            mocked_open.assert_called_once_with(self.image.file_name, 'rb')
            mock_file = vc.open_file
            tells = [0, self.image.size]
            mock_file.tell.side_effect = tells
            # open_file is not None, position == tell() + amount case of
            # seek_within_current_file();
            # tell() == size, len == 0 case of next_image_if_needed()
            vc.seek_within_current_file(self.image.size)
            mock_file.read.assert_not_called()
            self.callback.assert_not_called()
            self.assertEqual(mock_file.tell.call_count, len(tells))
            # Don't care whether seek() is called before closing
            seeks = mock_file.seek.call_count
            self.assertEqual(mock_file.close.call_count, 1)
            # open_file is None case of seek_within_current_file
            with self.assertRaises(IOError):
                vc.seek_within_current_file(1)
            mock_file.read.assert_not_called()
            self.callback.assert_not_called()
            self.assertEqual(mock_file.tell.call_count, len(tells))
            self.assertEqual(mock_file.seek.call_count, seeks)
            # self.open_file is None case of __exit__
            self.assertEqual(mock_file.close.call_count, 1)

    @patch.object(virtualcat, 'open', new_callable=mock_open)
    def test_multiple_images_skip_beyond_end(self, mocked_open):
        image_list = []
        call_list = []
        for i in range(5):
            image = MagicMock()
            image.file_name = '/file{}'.format(i)
            call_list.append(call(image.file_name, 'rb'))
            call_list.append(call().tell())
            image.size = i + 1
            call_list.append(call().seek(image.size))
            call_list.append(call().tell())
            call_list.append(call().close())
            image_list.append(image)
        call_list.append(call().close())
        with virtualcat.VirtualCat(image_list) as vc:
            for i in range(5):
                mock_file = vc.open_file
                mock_file.tell.side_effect = [0, i + 1]
                # open_file is not None, position == size case of
                # seek_within_current_file(); tell() == size, len == 0 case of
                # next_image_if_needed()
                vc.seek_within_current_file(10)
                self.callback.assert_not_called()
                # Don't care whether seek() is called before closing
        self.assertEqual(mocked_open.mock_calls, call_list)

    @patch.object(virtualcat, 'open', new_callable=mock_open)
    def test_multiple_images_peek_beyond_end(self, mocked_open):
        image = MagicMock()
        with virtualcat.VirtualCat([image]) as vc:
            mock_file = vc.open_file
            mock_file.tell.side_effect = [0, 1]
            mock_file.read.side_effect = [self.data]
            vc.peek()
            mock_file.read.side_effect = [b'']
            with self.assertRaises(IOError):
                vc.peek()
            self.callback.assert_not_called()
        self.assertEqual(mocked_open.mock_calls, [
            call(image.file_name, 'rb'),
            call().tell(),
            call().read(4),
            call().seek(-4, os.SEEK_CUR),
            call().tell(),
            call().read(4),
            call().close()
        ])
