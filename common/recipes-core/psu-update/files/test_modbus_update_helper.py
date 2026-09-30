import io
import unittest
from unittest.mock import patch

import modbus_update_helper
from modbus_update_helper import auto_int, bh, get_parser, print_perc, retry


class TestBh(unittest.TestCase):
    def test_bytes_to_hex_string(self):
        self.assertEqual(bh(b"\x2b\x71\x62"), "2b7162")
        self.assertEqual(bh(b""), "")
        self.assertEqual(bh(bytearray(b"\xff\x00")), "ff00")

    def test_result_is_str_not_bytes(self):
        # It is concatenated onto error messages, so it cannot be bytes.
        self.assertIsInstance(bh(b"\x01"), str)


class TestAutoInt(unittest.TestCase):
    def test_decimal(self):
        self.assertEqual(auto_int("10"), 10)

    def test_hex(self):
        self.assertEqual(auto_int("0x10"), 16)
        self.assertEqual(auto_int("0X10"), 16)

    def test_octal_and_binary(self):
        self.assertEqual(auto_int("0o17"), 15)
        self.assertEqual(auto_int("0b101"), 5)

    def test_negative(self):
        self.assertEqual(auto_int("-0x2"), -2)

    def test_rejects_junk(self):
        with self.assertRaises(ValueError):
            auto_int("ff")
        with self.assertRaises(ValueError):
            auto_int("")


class TestRetry(unittest.TestCase):
    def setUp(self):
        patcher = patch.object(modbus_update_helper.time, "sleep")
        self.sleep = patcher.start()
        self.addCleanup(patcher.stop)

    def test_returns_the_first_success_without_retrying(self):
        calls = []

        @retry(3, verbose=0)
        def fn(arg):
            calls.append(arg)
            return "ok"

        self.assertEqual(fn("a"), "ok")
        self.assertEqual(calls, ["a"])
        self.sleep.assert_not_called()

    def test_retries_until_it_succeeds(self):
        calls = []

        @retry(5, delay=1.0, verbose=0)
        def fn():
            calls.append(1)
            if len(calls) < 3:
                raise ValueError("not yet")
            return "ok"

        self.assertEqual(fn(), "ok")
        self.assertEqual(len(calls), 3)
        self.assertEqual(self.sleep.call_count, 2)
        self.sleep.assert_called_with(1.0)

    def test_the_last_attempt_is_not_swallowed(self):
        # After `times` failures it calls the function once more outside
        # the try, so the caller sees the real exception. That also means
        # times=N gives N+1 calls.
        calls = []

        @retry(3, verbose=0)
        def fn():
            calls.append(1)
            raise ValueError("always")

        with self.assertRaises(ValueError):
            fn()
        self.assertEqual(len(calls), 4)

    def test_only_the_listed_exceptions_are_retried(self):
        calls = []

        @retry(3, exceptions=(KeyError,), verbose=0)
        def fn():
            calls.append(1)
            raise ValueError("not retried")

        with self.assertRaises(ValueError):
            fn()
        self.assertEqual(len(calls), 1)

    def test_passes_arguments_and_returns_the_value_through(self):
        @retry(2, verbose=0)
        def fn(a, b=0):
            return a + b

        self.assertEqual(fn(1, b=2), 3)

    def test_quiet_when_verbose_is_zero(self):
        @retry(2, verbose=0)
        def fn():
            raise ValueError("boom")

        with patch("sys.stdout", new=io.StringIO()) as out:
            with self.assertRaises(ValueError):
                fn()
        self.assertEqual(out.getvalue(), "")

    def test_reports_each_failure_when_verbose(self):
        @retry(2, verbose=1)
        def fn():
            raise ValueError("boom")

        with patch("sys.stdout", new=io.StringIO()) as out:
            with self.assertRaises(ValueError):
                fn()
        self.assertEqual(out.getvalue().count("Exception: boom"), 2)


class FakeTTY(io.StringIO):
    def __init__(self, tty):
        super().__init__()
        self.tty = tty

    def isatty(self):
        return self.tty


class TestPrintPerc(unittest.TestCase):
    def output(self, tty, *args):
        out = FakeTTY(tty)
        with patch("sys.stdout", new=out):
            print_perc(*args)
        return out.getvalue()

    def test_says_nothing_when_not_a_tty(self):
        # Progress spam would otherwise fill the restapi log.
        self.assertEqual(self.output(False, 50.0, "Sending..."), "")

    def test_rewrites_the_same_line_while_in_progress(self):
        self.assertEqual(self.output(True, 12.5, "Sending..."), "\r[12.50%] Sending...")

    def test_ends_the_line_at_100_percent(self):
        self.assertEqual(self.output(True, 100.0, "Done"), "\r[100.00%] Done\n")

    def test_message_is_optional(self):
        self.assertEqual(self.output(True, 0.0), "\r[0.00%] ")


class TestGetParser(unittest.TestCase):
    def test_parses_an_address_and_a_file(self):
        args = get_parser().parse_args(["--addr", "0x32", "fw.bin"])
        self.assertEqual(args.addr, 0x32)
        self.assertEqual(args.file, "fw.bin")

    def test_address_is_required(self):
        with patch("sys.stderr", new=io.StringIO()):
            with self.assertRaises(SystemExit):
                get_parser().parse_args(["fw.bin"])

    def test_file_is_required(self):
        with patch("sys.stderr", new=io.StringIO()):
            with self.assertRaises(SystemExit):
                get_parser().parse_args(["--addr", "0x32"])

    def test_subcommands_can_add_their_own_arguments(self):
        parser = get_parser()
        parser.add_argument("--vendor", default="delta")
        args = parser.parse_args(["--addr", "1", "fw.bin", "--vendor", "panasonic"])
        self.assertEqual(args.vendor, "panasonic")


if __name__ == "__main__":
    unittest.main()
