import io
import struct
import unittest
from unittest.mock import patch

import psu_update_delta_orv3 as delta
from psu_update_delta_orv3 import (
    activate,
    BadMEIResponse,
    delta_seccalckey,
    erase_flash,
    get_challenge,
    get_status_reg,
    key_handshake,
    send_image,
    send_key,
    set_write_address,
    StatusRegister,
    update_psu,
    verify_flash,
    wait_status,
    write_data,
)

# Imported before the modules under test: it puts the fake pyrmd into
# sys.modules, which psu_update_delta_orv3 needs to be importable.
from test_mocks import FakeDevice

OK_RESP = b"\x2b\x71\x71\xff\xff\xff\xff\xff\xff"
ADDR_OK_RESP = b"\x2b\x71\x74\xff\xff\xff\xff\xff\xff"


def status_resp(value):
    return b"\x2b\x71\x62\x00\x00" + struct.pack(">L", value)


def bit(name):
    return 1 << StatusRegister._fields_.index(name)


class Segment:
    """The part of a hexfile.Segment send_image() uses"""

    def __init__(self, start_address, data):
        self.start_address = start_address
        self.data = data

    def __len__(self):
        return len(self.data)

    def __str__(self):
        return "<%d byte segment @ 0x%08x>" % (len(self.data), self.start_address)


class Image:
    __slots__ = ("segments",)

    def __init__(self, segments):
        self.segments = segments


class QuietTestCase(unittest.TestCase):
    def setUp(self):
        patcher = patch("sys.stdout", new=io.StringIO())
        self.stdout = patcher.start()
        self.addCleanup(patcher.stop)
        patcher = patch("time.sleep")
        self.sleep = patcher.start()
        self.addCleanup(patcher.stop)


class TestStatusRegister(unittest.TestCase):
    def test_built_from_an_int(self):
        self.assertTrue(StatusRegister(bit("WAIT"))["WAIT"])

    def test_built_from_the_four_bytes_off_the_wire(self):
        # Big endian, so the first byte on the wire is the top byte.
        status = StatusRegister(b"\x80\x00\x00\x00")
        self.assertTrue(status["ERROR_DETECTED"])
        self.assertFalse(status["WAIT"])

    def test_a_bit_in_each_byte(self):
        self.assertTrue(StatusRegister(bit("ERASE_DONE"))["ERASE_DONE"])
        self.assertTrue(StatusRegister(bit("CRC_VERIFIED"))["CRC_VERIFIED"])
        self.assertTrue(StatusRegister(bit("DEV_UPD_RDY"))["DEV_UPD_RDY"])
        self.assertTrue(StatusRegister(bit("SEQUENCE_ERROR"))["SEQUENCE_ERROR"])

    def test_str_lists_every_field(self):
        self.assertIn("('WAIT', True)", str(StatusRegister(1)))

    def test_an_unknown_field_name(self):
        with self.assertRaises(ValueError):
            StatusRegister(0)["NO_SUCH_BIT"]


class TestDeltaSeccalckey(unittest.TestCase):
    def test_the_response_is_four_bytes(self):
        response = delta_seccalckey(b"\x00\x00\x00\x01", 0x1122334455667788)
        self.assertEqual(len(response), 4)

    def test_a_known_challenge_response_pair(self):
        # Pinned against the algorithm as written: a change here locks
        # every PSU out.
        self.assertEqual(
            delta_seccalckey(b"\x12\x34\x56\x78", 0x1122334455667788),
            b"\x29\x78\xc3\x2a",
        )

    def test_the_upper_half_of_the_key_is_mixed_in_last(self):
        challenge = b"\x00\x00\x00\x00"
        self.assertEqual(
            delta_seccalckey(challenge, 0xAABBCCDD00000000), b"\xaa\xbb\xcc\xdd"
        )

    def test_different_challenges_give_different_responses(self):
        key = 0x1122334455667788
        self.assertNotEqual(
            delta_seccalckey(b"\x00\x00\x00\x01", key),
            delta_seccalckey(b"\x00\x00\x00\x02", key),
        )


class TestGetStatusReg(QuietTestCase):
    def test_asks_for_the_status_register(self):
        dev = FakeDevice(raw=[status_resp(bit("WAIT"))])
        status = get_status_reg(dev)
        self.assertEqual(dev.calls, [("raw", b"\x2b\x64\x22\x00\x00", 12, 0)])
        self.assertTrue(status["WAIT"])

    def test_a_reply_which_is_not_a_status_reply(self):
        dev = FakeDevice(raw=[b"\x2b\x71\x63\x00\x00\x00\x00\x00\x00"])
        with self.assertRaises(BadMEIResponse):
            get_status_reg(dev)

    def test_a_truncated_reply(self):
        dev = FakeDevice(raw=[b"\x2b\x71\x62\x00\x00"])
        with self.assertRaises(BadMEIResponse):
            get_status_reg(dev)


class TestWaitStatus(QuietTestCase):
    def test_returns_as_soon_as_the_bit_is_set(self):
        dev = FakeDevice(raw=[status_resp(0), status_resp(bit("ADD_ACCEPTED"))])
        status = wait_status(dev, bit_set="ADD_ACCEPTED", delay=0.1, timeout=10)
        self.assertTrue(status["ADD_ACCEPTED"])
        self.assertEqual(len(dev.calls), 2)

    def test_returns_as_soon_as_the_bit_is_cleared(self):
        dev = FakeDevice(raw=[status_resp(bit("SEND_DATA_BUSY")), status_resp(0)])
        wait_status(dev, bit_cleared="SEND_DATA_BUSY", delay=0.1, timeout=10)
        self.assertEqual(len(dev.calls), 2)

    def test_a_bit_which_never_arrives(self):
        dev = FakeDevice(raw=[status_resp(0)] * 10)
        with self.assertRaises(Exception) as ctx:
            wait_status(dev, bit_set="ADD_ACCEPTED", delay=1.0, timeout=5)
        self.assertNotIsInstance(ctx.exception, BadMEIResponse)
        self.assertIn("timeout after(sec): 5", self.stdout.getvalue())


class TestKeyHandshake(QuietTestCase):
    def test_the_challenge_is_answered_with_the_calculated_key(self):
        challenge = b"\x12\x34\x56\x78"
        dev = FakeDevice(
            raw=[
                b"\x2b\x71\x67\x00\x00" + challenge,
                b"\x2b\x71\x67\x00\x01\xff\xff\xff\xff",
            ]
        )
        key_handshake(dev, 0x1122334455667788)
        self.assertEqual(dev.calls[0][1], b"\x2b\x64\x27\x00\x00")
        self.assertEqual(
            dev.calls[1][1],
            b"\x2b\x64\x27\x00\x01" + delta_seccalckey(challenge, 0x1122334455667788),
        )

    def test_a_psu_which_does_not_offer_a_challenge(self):
        dev = FakeDevice(raw=[b"\x2b\x71\x68\x00\x00\x00\x00\x00\x00"])
        with self.assertRaises(BadMEIResponse):
            get_challenge(dev)

    def test_a_psu_which_rejects_the_key(self):
        dev = FakeDevice(raw=[b"\x2b\x71\x67\x00\x01\x00\x00\x00\x00"])
        with self.assertRaises(BadMEIResponse):
            send_key(dev, b"\x00\x00\x00\x00")


class TestEraseFlash(QuietTestCase):
    def test_erases_and_confirms(self):
        dev = FakeDevice(raw=[OK_RESP, status_resp(bit("ERASE_DONE"))])
        erase_flash(dev)
        self.assertEqual(dev.calls[0][1], b"\x2b\x64\x31\x00\x00\xff\xff\xff\xff")
        self.assertIn("Erase successful", self.stdout.getvalue())

    def test_a_psu_which_does_not_take_the_erase_command(self):
        dev = FakeDevice(raw=[b"\x2b\x71\x72\xff\xff\xff\xff\xff\xff"])
        with self.assertRaises(BadMEIResponse):
            erase_flash(dev)

    def test_a_flash_which_did_not_erase(self):
        dev = FakeDevice(raw=[OK_RESP, status_resp(bit("ERASE_FAIL"))])
        with self.assertRaises(Exception) as ctx:
            erase_flash(dev)
        self.assertIn("ERASE_FAIL", str(ctx.exception))


class TestSetWriteAddress(QuietTestCase):
    def test_sends_the_flash_address_and_waits_for_it_to_be_accepted(self):
        dev = FakeDevice(raw=[ADDR_OK_RESP, status_resp(bit("ADD_ACCEPTED"))])
        set_write_address(dev, 0x08001000)
        self.assertEqual(dev.calls[0][1], b"\x2b\x64\x34\x00\x00\x08\x00\x10\x00")

    def test_an_address_the_psu_will_not_take(self):
        dev = FakeDevice(raw=[b"\x2b\x71\x75\xff\xff\xff\xff\xff\xff"])
        with self.assertRaises(BadMEIResponse):
            set_write_address(dev, 0)


class TestWriteData(QuietTestCase):
    CHUNK = bytearray(range(128))

    def test_a_chunk_which_is_ready_straight_away(self):
        dev = FakeDevice(
            raw=[
                b"\x2b\x73\x76\xff\xff\xff\xff\xff\xff",
                status_resp(bit("SEND_DATA_RDY")),
            ]
        )
        write_data(dev, self.CHUNK)
        self.assertEqual(dev.calls[0][1], b"\x2b\x65\x36" + self.CHUNK)

    def test_a_chunk_which_needs_a_second_wait(self):
        dev = FakeDevice(
            raw=[
                b"\x2b\x73\x76\xff\xff\xff\xff\xff\xff",
                status_resp(0),  # not busy, not ready
                status_resp(bit("SEND_DATA_RDY")),
            ]
        )
        write_data(dev, self.CHUNK)
        self.assertEqual(len(dev.calls), 3)

    def test_only_a_full_chunk_may_be_written(self):
        with self.assertRaises(AssertionError):
            write_data(FakeDevice(), bytearray(64))

    def test_a_psu_which_does_not_take_the_chunk(self):
        dev = FakeDevice(raw=[b"\x2b\x73\x77\xff\xff\xff\xff\xff\xff"])
        with self.assertRaises(BadMEIResponse):
            write_data(dev, self.CHUNK)

    def test_a_psu_stuck_busy(self):
        dev = FakeDevice(
            raw=[b"\x2b\x73\x76\xff\xff\xff\xff\xff\xff"]
            + [status_resp(bit("SEND_DATA_BUSY"))] * 200
        )
        with self.assertRaisesRegex(Exception, "'SEND_DATA_BUSY', True"):
            write_data(dev, self.CHUNK)


class TestVerifyAndActivate(QuietTestCase):
    def test_verify_waits_for_the_crc_check(self):
        dev = FakeDevice(raw=[OK_RESP, status_resp(bit("CRC_VERIFIED"))])
        verify_flash(dev)
        self.assertEqual(dev.calls[0][1], b"\x2b\x64\x31\x00\x01")
        self.assertIn("Verify of flash successful!", self.stdout.getvalue())

    def test_an_image_which_does_not_verify(self):
        dev = FakeDevice(raw=[OK_RESP, status_resp(bit("CRC_WRONG"))])
        with self.assertRaises(Exception) as ctx:
            verify_flash(dev)
        self.assertIn("CRC_WRONG", str(ctx.exception))

    def test_activate_switches_to_the_new_image(self):
        dev = FakeDevice(raw=[b"\x2b\x71\x6e\xff\xff\xff\xff\xff\xff"])
        activate(dev)
        self.assertEqual(dev.calls[0][1], b"\x2b\x64\x2e\x00\x00")

    def test_a_psu_which_will_not_activate(self):
        dev = FakeDevice(raw=[b"\x2b\x71\x6f\xff\xff\xff\xff\xff\xff"])
        with self.assertRaises(BadMEIResponse):
            activate(dev)


class TestSendImage(QuietTestCase):
    def responses(self, chunks):
        replies = []
        for _ in range(chunks):
            replies.append(b"\x2b\x73\x76\xff\xff\xff\xff\xff\xff")
            replies.append(status_resp(bit("SEND_DATA_RDY")))
        return replies

    def test_each_segment_sets_its_own_write_address(self):
        image = Image(
            [
                Segment(0x08000000, bytes(128)),
                Segment(0x08010000, bytes(128)),
            ]
        )
        addr_replies = [
            b"\x2b\x71\x74\xff\xff\xff\xff\xff\xff",
            status_resp(bit("ADD_ACCEPTED")),
        ]
        dev = FakeDevice(
            raw=addr_replies + self.responses(1) + addr_replies + self.responses(1)
        )
        send_image(dev, image)
        addr_cmds = [c[1] for c in dev.calls if c[1][:3] == b"\x2b\x64\x34"]
        self.assertEqual(
            addr_cmds,
            [
                b"\x2b\x64\x34\x00\x00\x08\x00\x00\x00",
                b"\x2b\x64\x34\x00\x00\x08\x01\x00\x00",
            ],
        )

    def test_a_short_last_chunk_is_padded_with_erased_flash(self):
        image = Image([Segment(0x08000000, bytes(130))])
        dev = FakeDevice(
            raw=[ADDR_OK_RESP, status_resp(bit("ADD_ACCEPTED"))] + self.responses(2)
        )
        send_image(dev, image)
        last = [c[1] for c in dev.calls if c[1][:3] == b"\x2b\x65\x36"][-1]
        self.assertEqual(last[3:], bytes(2) + b"\xff" * 126)

    def test_an_empty_segment_is_skipped(self):
        image = Image([Segment(0x08000000, b"")])
        dev = FakeDevice()
        send_image(dev, image)
        self.assertEqual(dev.calls, [])
        self.assertIn("Ignoring empty segment", self.stdout.getvalue())


class TestUpdatePsu(QuietTestCase):
    def test_the_whole_sequence(self):
        image = Image([Segment(0x08000000, bytes(128))])
        dev = FakeDevice(
            raw=[
                b"\x2b\x71\x67\x00\x00\x12\x34\x56\x78",  # challenge
                b"\x2b\x71\x67\x00\x01\xff\xff\xff\xff",  # key accepted
                OK_RESP,  # erase
                status_resp(bit("ERASE_DONE")),
                b"\x2b\x71\x74\xff\xff\xff\xff\xff\xff",  # set address
                status_resp(bit("ADD_ACCEPTED")),
                b"\x2b\x73\x76\xff\xff\xff\xff\xff\xff",  # write chunk
                status_resp(bit("SEND_DATA_RDY")),
                OK_RESP,  # verify
                status_resp(bit("CRC_VERIFIED")),
                b"\x2b\x71\x6e\xff\xff\xff\xff\xff\xff",  # activate
            ]
        )
        with patch.object(delta.hexfile, "load", return_value=image):
            update_psu(dev, "fw.hex", 0x1122334455667788)
        self.assertEqual(len(dev.calls), 11)


class TestMain(QuietTestCase):
    def test_reports_the_version_before_and_after(self):
        dev = FakeDevice(read_str=["1.0", "2.0"])
        with patch.object(delta, "update_psu") as update:
            delta.main(dev, "fw.hex", key=0x1122334455667788)
        update.assert_called_once_with(dev, "fw.hex", 0x1122334455667788)
        self.assertEqual(dev.calls_of("read_str"), [("read_str", 48, 4, 0)] * 2)
        self.assertIn("Upgrade Success!", self.stdout.getvalue())

    def test_without_a_key_the_build_in_one_is_used(self):
        dev = FakeDevice(read_str=["1.0", "2.0"])
        with patch.object(delta.delta_key, "key", 0x99):
            with patch.object(delta, "update_psu") as update:
                delta.main(dev, "fw.hex")
        update.assert_called_once_with(dev, "fw.hex", 0x99)

    def test_a_build_without_a_key_cannot_update_a_delta_psu(self):
        with patch.object(delta.delta_key, "key", None):
            with self.assertRaises(SystemExit) as ctx:
                delta.main(FakeDevice(), "fw.hex")
        self.assertEqual(ctx.exception.code, 1)
        self.assertIn("Update Key is needed", self.stdout.getvalue())

    def test_a_failed_update_dumps_the_status_register(self):
        dev = FakeDevice(read_str=["1.0"], raw=[status_resp(bit("ERROR_DETECTED"))])
        with patch.object(delta, "update_psu", side_effect=ValueError("boom")):
            with patch("sys.stderr", new=io.StringIO()):
                with self.assertRaises(SystemExit) as ctx:
                    delta.main(dev, "fw.hex", key=1)
        self.assertEqual(ctx.exception.code, 1)
        self.assertIn("Status register dump:", self.stdout.getvalue())
        self.assertIn("('ERROR_DETECTED', True)", self.stdout.getvalue())


if __name__ == "__main__":
    unittest.main()
