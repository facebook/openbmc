import argparse
import sys
import time

from binascii import hexlify
from contextlib import contextmanager

from pyrmd import RackmonInterface as rmd


def retry(times, exceptions=(Exception), delay=0.0, verbose=1):
    def decorator(func):
        def newfn(*args, **kwargs):
            retries = 0
            while retries < times:
                try:
                    if verbose >= 2:
                        print("attempt %d of %d of %s" % (retries, times, str(func)))
                    return func(*args, **kwargs)
                except exceptions as e:
                    if verbose >= 1:
                        print(
                            "Exception: %s thrown on attempt %d of %d of %s"
                            % (str(e), retries, times, str(func))
                        )
                    retries += 1
                    time.sleep(delay)
            return func(*args, **kwargs)

        return newfn

    return decorator


def bh(bs):
    """bytes to hex *str*"""
    return hexlify(bs).decode("ascii")


def auto_int(x):
    """Auto converts string to int"""
    return int(x, 0)


def print_perc(x, opt_str=""):
    """Prints a perc-status line if run as TTY"""
    # Only print perc if we are a TTY.
    if sys.stdout.isatty():
        if x < 100.0:
            print("\r[%.2f%%] " % (x) + opt_str, end="")
        else:
            print("\r[%.2f%%] " % (x) + opt_str)
        sys.stdout.flush()


def get_parser():
    """Adds basic args common to all PSU/BBU/RPU updaters"""
    parser = argparse.ArgumentParser()
    parser.add_argument("--addr", type=auto_int, required=True, help="Modbus Address")
    parser.add_argument("file", help="firmware file")
    return parser


def get_pmm_addr(addr):
    pmm_addrs = {
        0x10: [[0x30, 0x35]],
        0x11: [[0x3A, 0x3F]],
        0x12: [[0x5A, 0x5F]],
        0x13: [[0x6A, 0x6F]],
        0x14: [[0x70, 0x75]],
        0x15: [[0x7A, 0x7F]],
        0x16: [[0x80, 0x85]],
        0x17: [[0x36, 0x39], [0x46, 0x47]],
        0x18: [[0x56, 0x58], [0x66, 0x68]],
        0x19: [[0x76, 0x78], [0x87, 0x88]],
        0x1A: [[0x8A, 0x8F]],
        0x20: [[0x90, 0x95]],
        0x21: [[0x9A, 0x9F]],
        0x22: [[0xAA, 0xAF]],
        0x23: [[0xBA, 0xBF]],
        0x24: [[0xD0, 0xD5]],
        0x25: [[0xDA, 0xDF]],
        0x26: [[0xE0, 0xE5]],
        0x27: [[0x96, 0x98], [0xA7, 0xA9]],
        0x28: [[0xB0, 0xB3], [0xB7, 0xB9]],
        0x29: [[0xE6, 0xE7], [0xF6, 0xF9]],
        0x2A: [[0xFA, 0xFF]],
    }
    for pmm_addr, addr_ranges in pmm_addrs.items():
        for addr_range in addr_ranges:
            if addr >= addr_range[0] and addr <= addr_range[1]:
                return pmm_addr
    return None


def control_pmm_monitoring(addr, pause):
    pmm_pause_reg = 0x7B
    pmm_addr = get_pmm_addr(addr)
    if pmm_addr is None:
        return
    try:
        print(f"Setting PMM{pmm_addr} Monitoring pause: {pause}")
        rmd.write(pmm_addr, pmm_pause_reg, pause)
    except Exception:
        print(f"WARNING: Control PMM:{pmm_pause_reg} {pause} failed")


@contextmanager
def suppress_monitoring(addr=None):
    """
    contextmanager to pause rackmond monitoring on entry
    and resume on exit, including exits due to exception
    """
    try:
        print("Pausing monitoring...")
        rmd.pause()
        if addr is not None:
            control_pmm_monitoring(addr, 0x1)
        yield
    finally:
        if addr is not None:
            control_pmm_monitoring(addr, 0x0)
        print("Resuming monitoring...")
        rmd.resume()
        time.sleep(10.0)
