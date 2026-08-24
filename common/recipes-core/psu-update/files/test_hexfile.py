import os
import tempfile
import unittest

import hexfile
from hexfile import long, Segment, short

DATA = 0
END_OF_FILE = 1
EXT_SEGMENT_ADDR = 2
START_SEGMENT_ADDR = 3
EXT_LINEAR_ADDR = 4
START_LINEAR_ADDR = 5


def record(record_type, address, data=b"", byte_count=None):
    """One Intel HEX record, with a correct checksum"""
    if byte_count is None:
        byte_count = len(data)
    body = (
        bytes([byte_count]) + address.to_bytes(2, "big") + bytes([record_type]) + data
    )
    checksum = (-sum(body)) & 0xFF
    return ":" + (body + bytes([checksum])).hex().upper()


def eof():
    return record(END_OF_FILE, 0)


class HexFileTestCase(unittest.TestCase):
    def load(self, *lines):
        with tempfile.NamedTemporaryFile("w", suffix=".hex", delete=False) as f:
            f.write("\n".join(lines) + "\n")
            path = f.name
        self.addCleanup(os.unlink, path)
        return hexfile.load(path)


class TestWordHelpers(unittest.TestCase):
    def test_short(self):
        self.assertEqual(short(0x12, 0x34), 0x1234)
        self.assertEqual(short(0, 0), 0)
        self.assertEqual(short(0xFF, 0xFF), 0xFFFF)

    def test_long(self):
        self.assertEqual(long(0x12, 0x34, 0x56, 0x78), 0x12345678)
        self.assertEqual(long(0, 0, 0, 1), 1)


class TestLoad(HexFileTestCase):
    def test_a_single_data_record(self):
        img = self.load(record(DATA, 0x0100, b"\xde\xad\xbe\xef"), eof())
        self.assertEqual(len(img.segments), 1)
        self.assertEqual(img.segments[0].start_address, 0x0100)
        self.assertEqual(list(img.segments[0].data), [0xDE, 0xAD, 0xBE, 0xEF])
        self.assertEqual(len(img), 4)
        self.assertEqual(img.size, 4)

    def test_contiguous_records_join_into_one_segment(self):
        img = self.load(
            record(DATA, 0x0100, b"\x01\x02"),
            record(DATA, 0x0102, b"\x03\x04"),
            eof(),
        )
        self.assertEqual(len(img.segments), 1)
        self.assertEqual(list(img.segments[0].data), [1, 2, 3, 4])
        self.assertEqual(img.segments[0].end_address, 0x0104)

    def test_a_gap_starts_a_new_segment(self):
        img = self.load(
            record(DATA, 0x0100, b"\x01\x02"),
            record(DATA, 0x0200, b"\x03\x04"),
            eof(),
        )
        self.assertEqual([s.start_address for s in img.segments], [0x0100, 0x0200])
        self.assertEqual(len(img), 4)

    def test_extended_linear_address_moves_the_next_records_up(self):
        img = self.load(
            record(EXT_LINEAR_ADDR, 0, b"\x00\x01"),
            record(DATA, 0x0100, b"\x01"),
            eof(),
        )
        self.assertEqual(img.segments[0].start_address, 0x00010100)

    def test_extended_segment_address_is_shifted_by_four(self):
        img = self.load(
            record(EXT_SEGMENT_ADDR, 0, b"\x10\x00"),
            record(DATA, 0x0010, b"\x01"),
            eof(),
        )
        self.assertEqual(img.segments[0].start_address, 0x10010)

    def test_start_linear_address_is_the_entry_point(self):
        img = self.load(
            record(START_LINEAR_ADDR, 0, b"\x00\x01\x02\x03"),
            eof(),
        )
        self.assertEqual(img.eip, 0x00010203)
        self.assertIsNone(img.cs)

    def test_start_segment_address_is_a_cs_ip_pair(self):
        img = self.load(
            record(START_SEGMENT_ADDR, 0, b"\x12\x34\x56\x78"),
            eof(),
        )
        self.assertEqual(img.cs, 0x1234)
        self.assertEqual(img.ip, 0x5678)
        self.assertIsNone(img.eip)

    def test_lines_which_are_not_records_are_ignored(self):
        img = self.load(
            "# a comment",
            "",
            "   " + record(DATA, 0x0100, b"\x01") + "  ",
            eof(),
        )
        self.assertEqual(len(img), 1)

    def test_a_record_after_the_end_of_file_is_rejected(self):
        with self.assertRaises(Exception) as ctx:
            self.load(eof(), record(DATA, 0x0100, b"\x01"))
        self.assertIn("after end of file", str(ctx.exception))

    def test_a_bad_checksum_is_rejected(self):
        line = record(DATA, 0x0100, b"\x01")
        corrupt = line[:-2] + ("%02X" % ((int(line[-2:], 16) + 1) & 0xFF))
        with self.assertRaises(Exception) as ctx:
            self.load(corrupt, eof())
        self.assertIn("checksum", str(ctx.exception))

    def test_a_data_record_which_lies_about_its_size_is_rejected(self):
        with self.assertRaises(Exception) as ctx:
            self.load(record(DATA, 0x0100, b"\x01\x02", byte_count=4), eof())
        self.assertIn("size", str(ctx.exception))

    def test_an_address_record_which_lies_about_its_size_is_rejected(self):
        cases = [
            (EXT_SEGMENT_ADDR, b"\x10"),
            (EXT_LINEAR_ADDR, b"\x00\x01\x02"),
            (START_LINEAR_ADDR, b"\x00\x01"),
            (START_SEGMENT_ADDR, b"\x00"),
        ]
        for record_type, data in cases:
            with self.subTest(record_type=record_type):
                with self.assertRaisesRegex(Exception, "Byte count misreported"):
                    self.load(record(record_type, 0, data), eof())

    def test_an_unknown_record_type_is_rejected(self):
        with self.assertRaises(Exception) as ctx:
            self.load(record(6, 0, b""), eof())
        self.assertIn("Unknown record type", str(ctx.exception))

    def test_the_error_names_the_line(self):
        line = record(DATA, 0x0100, b"\x01")
        corrupt = line[:-2] + ("%02X" % ((int(line[-2:], 16) + 1) & 0xFF))
        with self.assertRaises(Exception) as ctx:
            self.load("# comment", corrupt, eof())
        self.assertIn("line 2", str(ctx.exception))

    def test_a_file_with_no_records_at_all(self):
        img = self.load("")
        self.assertEqual(img.segments, [])
        self.assertEqual(len(img), 0)


class TestHexFileAccess(HexFileTestCase):
    def setUp(self):
        self.img = self.load(
            record(DATA, 0x0100, b"\x01\x02\x03\x04"),
            record(DATA, 0x0200, b"\x05\x06"),
            eof(),
        )

    def test_indexing_by_address(self):
        self.assertEqual(self.img[0x0100], 0x01)
        self.assertEqual(self.img[0x0103], 0x04)
        self.assertEqual(self.img[0x0201], 0x06)

    def test_an_address_in_no_segment(self):
        with self.assertRaises(IndexError):
            self.img[0x0150]

    def test_iterating_yields_address_value_pairs_of_every_segment(self):
        self.assertEqual(
            list(self.img),
            [
                (0x0100, 1),
                (0x0101, 2),
                (0x0102, 3),
                (0x0103, 4),
                (0x0200, 5),
                (0x0201, 6),
            ],
        )

    def test_len_is_the_total_of_the_segments(self):
        self.assertEqual(len(self.img), 6)

    def test_pretty_string_lists_every_segment(self):
        text = self.img.pretty_string()
        self.assertIn("Segment @ 0x00000100 (4 bytes)", text)
        self.assertIn("00000100 01 02 03 04", text)
        self.assertIn("Segment @ 0x00000200 (2 bytes)", text)

    def test_pretty_string_reports_the_entry_point(self):
        img = self.load(record(START_LINEAR_ADDR, 0, b"\x00\x01\x02\x03"), eof())
        self.assertIn("EIP 0x10203", img.pretty_string())


class TestSegment(unittest.TestCase):
    def setUp(self):
        self.segment = Segment(0x100, [1, 2, 3, 4])

    def test_an_empty_segment(self):
        empty = Segment(0x100)
        self.assertEqual(empty.data, [])
        self.assertEqual(empty.size, 0)
        self.assertEqual(empty.end_address, 0x100)

    def test_bounds(self):
        self.assertEqual(self.segment.end_address, 0x104)
        self.assertEqual(self.segment.size, 4)
        self.assertEqual(len(self.segment), 4)
        self.assertEqual(list(self.segment.addresses), [0x100, 0x101, 0x102, 0x103])

    def test_contains_is_half_open(self):
        self.assertIn(0x100, self.segment)
        self.assertIn(0x103, self.segment)
        self.assertNotIn(0x104, self.segment)
        self.assertNotIn(0xFF, self.segment)

    def test_indexing_is_by_address(self):
        self.assertEqual(self.segment[0x102], 3)
        with self.assertRaises(IndexError):
            self.segment[0x104]

    def test_slicing_returns_the_bytes_in_the_range(self):
        # The returned Segment's start_address is the requested address
        # plus this segment's own start: a quirk of the vendored code
        # which nothing in the updaters relies on, pinned so a change is
        # deliberate.
        sliced = self.segment[0x101:0x103]
        self.assertEqual(sliced.data, [2, 3])
        self.assertEqual(sliced.start_address, 0x201)

    def test_slicing_past_the_end(self):
        with self.assertRaises(IndexError):
            self.segment[0x102:0x110]

    def test_str(self):
        self.assertEqual(str(self.segment), "<4 byte segment @ 0x00000100>")
        self.assertEqual(repr(self.segment), str(self.segment))

    def test_pretty_string_wraps_at_the_stride(self):
        self.assertEqual(
            Segment(0, list(range(4))).pretty_string(stride=2),
            "00000000 00 01\n00000002 02 03",
        )

    def test_iterating_yields_address_value_pairs(self):
        self.assertEqual(list(self.segment)[0], (0x100, 1))


class TestHexFileIsIndexableAcrossSegments(HexFileTestCase):
    def test_a_slice_is_served_by_the_segment_holding_its_start(self):
        img = self.load(record(DATA, 0x0100, b"\x01\x02\x03\x04"), eof())
        self.assertEqual(img[0x0100:0x0102].data, [1, 2])


if __name__ == "__main__":
    unittest.main()
