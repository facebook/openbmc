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


@contextmanager
def suppress_monitoring():
    """
    contextmanager to pause rackmond monitoring on entry
    and resume on exit, including exits due to exception
    """
    try:
        print("Pausing monitoring...")
        rmd.pause()
        yield
    finally:
        print("Resuming monitoring...")
        rmd.resume()
        time.sleep(10.0)
