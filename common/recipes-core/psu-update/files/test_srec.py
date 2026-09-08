import os
import tempfile
import unittest

import srec
from srec import hx, Image, schecksum, Section, SRecord


def line(stype, addr, data=b""):
    """One S-record, with a correct checksum"""
    addrlen = 1 + stype if stype in (1, 2, 3) else 2
    body = addr.to_bytes(addrlen, "big") + data
    rec = bytes([len(body) + 1]) + body
    return "S%d%s%02X" % (stype, hx(rec).upper(), schecksum(rec))


class TestHelpers(unittest.TestCase):
    def test_hx(self):
        self.assertEqual(hx(b"\x0a\xff"), "0aff")
        self.assertEqual(hx(b""), "")

    def test_schecksum_is_the_ones_complement_of_the_low_byte(self):
        self.assertEqual(schecksum(b"\x00"), 0xFF)
        self.assertEqual(schecksum(b"\xff"), 0x00)
        self.assertEqual(schecksum(b"\x01\x02"), 0xFC)


class TestSection(unittest.TestCase):
    def setUp(self):
        self.section = Section(0x100)
        self.section.data.extend(b"\x01\x02\x03\x04")

    def test_bounds(self):
        self.assertEqual(self.section.start, 0x100)
        self.assertEqual(self.section.end, 0x104)

    def test_contains_is_half_open(self):
        self.assertTrue(self.section.contains(0x100))
        self.assertTrue(self.section.contains(0x103))
        self.assertFalse(self.section.contains(0x104))
        self.assertFalse(self.section.contains(0xFF))

    def test_slices_take_absolute_addresses(self):
        self.assertEqual(self.section[0x101:0x103], bytearray(b"\x02\x03"))

    def test_a_slice_past_the_end_is_truncated(self):
        self.assertEqual(self.section[0x102:0x200], bytearray(b"\x03\x04"))

    def test_indexing_by_address_is_not_supported(self):
        with self.assertRaisesRegex(Exception, "must use a slice"):
            self.section[0x100]


class TestSRecord(unittest.TestCase):
    def test_end_is_past_the_last_byte(self):
        self.assertEqual(SRecord(1, 0x100, b"\x01\x02").end, 0x102)


class TestParse(unittest.TestCase):
    def parse(self, *lines, **kwargs):
        with tempfile.NamedTemporaryFile("w", suffix=".S", delete=False) as f:
            f.write("\n".join(lines) + "\n")
            path = f.name
        self.addCleanup(os.unlink, path)
        return list(srec.parse(path, **kwargs))

    def test_a_16_bit_data_record(self):
        (rec,) = self.parse(line(1, 0x0100, b"\xde\xad"))
        self.assertEqual((rec.stype, rec.addr, rec.data), (1, 0x0100, b"\xde\xad"))

    def test_a_24_bit_data_record(self):
        (rec,) = self.parse(line(2, 0x010000, b"\x01"))
        self.assertEqual((rec.stype, rec.addr), (2, 0x010000))

    def test_a_32_bit_data_record(self):
        (rec,) = self.parse(line(3, 0x08000000, b"\x01"))
        self.assertEqual((rec.stype, rec.addr), (3, 0x08000000))

    def test_the_header_record_is_skipped(self):
        recs = self.parse(line(0, 0, b"HDR"), line(1, 0x100, b"\x01"))
        self.assertEqual(len(recs), 1)

    def test_lines_which_are_not_records_are_skipped(self):
        recs = self.parse("", "# comment", "  " + line(1, 0x100, b"\x01") + "  ")
        self.assertEqual(len(recs), 1)

    def test_addresses_can_be_scaled_to_bytes(self):
        # Some devices count words where the S-record counts bytes.
        (rec,) = self.parse(line(1, 0x0100, b"\x01"), address_scale=2)
        self.assertEqual(rec.addr, 0x0200)

    def test_a_bad_checksum_is_rejected(self):
        good = line(1, 0x0100, b"\x01")
        corrupt = good[:-2] + ("%02X" % ((int(good[-2:], 16) + 1) & 0xFF))
        with self.assertRaises(Exception) as ctx:
            self.parse(corrupt)
        self.assertIn("Checksum", str(ctx.exception))

    def test_an_unhandled_record_type_is_rejected(self):
        with self.assertRaises(Exception) as ctx:
            self.parse(line(1, 0x100, b"\x01"), "S9030000FC")
        self.assertIn("Unhandled record", str(ctx.exception))

    def test_records_are_produced_lazily(self):
        # parse() is a generator, so a broken record only surfaces when
        # the caller gets that far.
        good = line(1, 0x0100, b"\x01")
        with tempfile.NamedTemporaryFile("w", suffix=".S", delete=False) as f:
            f.write(good + "\nS9030000FC\n")
            path = f.name
        self.addCleanup(os.unlink, path)
        records = srec.parse(path)
        self.assertEqual(next(records).addr, 0x0100)
        with self.assertRaisesRegex(Exception, "Unhandled record"):
            next(records)


class TestImageFromRecs(unittest.TestCase):
    def test_contiguous_records_become_one_section(self):
        img = Image.from_recs(
            [SRecord(1, 0x100, b"\x01\x02"), SRecord(1, 0x102, b"\x03")]
        )
        self.assertEqual(len(img.sections), 1)
        self.assertEqual(img.sections[0].data, bytearray(b"\x01\x02\x03"))

    def test_a_gap_starts_a_new_section(self):
        img = Image.from_recs([SRecord(1, 0x100, b"\x01"), SRecord(1, 0x200, b"\x02")])
        self.assertEqual([s.start for s in img.sections], [0x100, 0x200])

    def test_overlapping_records_are_rejected(self):
        with self.assertRaises(Exception) as ctx:
            Image.from_recs(
                [SRecord(1, 0x100, b"\x01\x02\x03"), SRecord(1, 0x101, b"\x04")]
            )
        self.assertIn("overlap", str(ctx.exception))

    def test_no_records_at_all(self):
        img = Image.from_recs([])
        self.assertEqual(img.sections, [])
        self.assertEqual(str(img), "Image<0 sections>")

    def test_str_counts_the_sections(self):
        img = Image.from_recs([SRecord(1, 0x100, b"\x01"), SRecord(1, 0x200, b"\x02")])
        self.assertEqual(str(img), "Image<2 sections>")


class TestImageSlicing(unittest.TestCase):
    def setUp(self):
        self.img = Image.from_recs(
            [
                SRecord(1, 0x100, b"\x01\x02\x03\x04"),
                SRecord(1, 0x200, b"\x05\x06"),
            ]
        )

    def test_within_one_section(self):
        self.assertEqual(self.img[0x100:0x102], bytearray(b"\x01\x02"))
        self.assertEqual(self.img[0x102:0x104], bytearray(b"\x03\x04"))

    def test_a_gap_reads_as_erased_flash(self):
        self.assertEqual(
            self.img[0x102:0x202],
            bytearray(b"\x03\x04" + b"\xff" * 0xFC + b"\x05\x06"),
        )

    def test_a_read_which_starts_in_a_gap(self):
        self.assertEqual(self.img[0x104:0x106], bytearray(b"\xff\xff"))

    def test_past_the_end_of_the_image(self):
        with self.assertRaises(Exception) as ctx:
            self.img[0x200:0x300]
        self.assertIn("past end of image", str(ctx.exception))

    def test_an_index_must_be_a_slice(self):
        with self.assertRaisesRegex(Exception, "must use slices"):
            self.img[0x100]

    def test_a_slice_cannot_step(self):
        with self.assertRaisesRegex(Exception, "cannot step"):
            self.img[0x100:0x104:2]


class TestImageDump(unittest.TestCase):
    def test_sections_are_written_at_their_own_offsets(self):
        img = Image.from_recs([SRecord(1, 0, b"\x01\x02"), SRecord(1, 8, b"\x03")])
        path = tempfile.mktemp(suffix=".bin")
        self.addCleanup(lambda: os.path.exists(path) and os.unlink(path))
        img.dump(path)
        with open(path, "rb") as f:
            self.assertEqual(f.read(), b"\x01\x02" + b"\x00" * 6 + b"\x03")

    def test_pickle_round_trips_an_image(self):
        import pickle

        img = Image.from_recs([SRecord(1, 0x100, b"\x01\x02")])
        path = tempfile.mktemp(suffix=".pickle")
        self.addCleanup(lambda: os.path.exists(path) and os.unlink(path))
        img.pickle(path)
        with open(path, "rb") as f:
            loaded = pickle.load(f)
        self.assertEqual(loaded[0x100:0x102], bytearray(b"\x01\x02"))


if __name__ == "__main__":
    unittest.main()
