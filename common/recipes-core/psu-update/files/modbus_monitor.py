"""
Whoever else is polling the bus, and how to make it stand off.

This is deliberately kept apart from the modbus backends
(modbus_impl_*.py): which transport we talk to a device with and who
monitors that device are independent choices. A device polled by
rackmond can still be driven over minimalmodbus, so pick a Monitor to
match the daemon which owns the bus, not the backend:

    dev = Modbus(addr, baud, parity, devpath, monitor=RackmonMonitor())

Every backend takes a monitor= argument and defaults to the one which
normally goes with it.
"""

import time
from contextlib import contextmanager, ExitStack

from modbus_common import PMM_PAUSE_REG

__all__ = [
    "Monitor",
    "NullMonitor",
    "MonitorChain",
    "PmmMonitor",
    "RackmonMonitor",
]

# Allow rackmon monitoring threads to exit
RACKMON_SETTLE_SECS = 5.0
# Allow any monitoring threads in PMM to exit
PMM_SETTLE_SECS = 1.0

# Request PMM to pause monitoring by writing 0x1 to PMM_PAUSE_REG
PMM_PAUSE_MONITORING = 0x1
# Request PMM to resume monitoring by writhing 0x0 to PMM_PAUSE_REG
PMM_RESUME_MONITORING = 0x0


class Monitor:
    """
    Something which polls devices on the bus and has to be told to stand
    off while we drive it ourselves.

    Subclasses implement pause()/resume(); everyone uses suppress().
    """

    def pause(self):
        pass

    def resume(self):
        pass

    @contextmanager
    def suppress(self):
        """
        Pause monitoring on entry and resume it on exit, including exits
        due to exception.
        """
        self.pause()
        try:
            yield
        finally:
            self.resume()


class NullMonitor(Monitor):
    """Nothing is watching this device, so nothing to suppress."""


class MonitorChain(Monitor):
    """
    Several monitors suppressed as one, in order, e.g. the daemon
    polling the port plus the PMM polling the device.

    Nones are dropped, so callers can pass an optional monitor straight
    in. Each monitor is resumed by its own suppress(), so one which
    fails to pause leaves the ones before it resumed and the ones after
    it untouched.
    """

    def __init__(self, *monitors):
        self.monitors = [m for m in monitors if m is not None]

    @contextmanager
    def suppress(self):
        with ExitStack() as stack:
            for monitor in self.monitors:
                stack.enter_context(monitor.suppress())
            yield


class PmmMonitor(Monitor):
    """
    A PMM monitors the devices behind it, independently of whatever
    monitors the PMM itself. pmm is a modbus handle to the PMM, of
    whichever backend the device behind it uses.

    Failing to pause it is a warning rather than an error: it degrades
    the update, it does not corrupt it.
    """

    def __init__(self, pmm):
        self.pmm = pmm

    def _set_pause(self, pause):
        try:
            self.pmm.write(PMM_PAUSE_REG, pause)
        except Exception:
            print(f"WARNING: Control PMM:{self.pmm.dev_addr} {pause} failed")

    def pause(self):
        print(f"Pausing PMM {self.pmm.dev_addr} monitoring...")
        self._set_pause(PMM_PAUSE_MONITORING)
        # Allow the PMM to fully pause before we start fw upgrade
        time.sleep(PMM_SETTLE_SECS)

    def resume(self):
        print(f"Resuming PMM {self.pmm.dev_addr} monitoring...")
        self._set_pause(PMM_RESUME_MONITORING)


class RackmonMonitor(Monitor):
    """rackmond, paused over its own interface."""

    def __init__(self):
        # Imported here rather than at module scope so that a system
        # which only has the minimalmodbus backend does not need
        # rackmon installed to import this module.
        import pyrmd

        self.rmd = pyrmd.RackmonInterface

    def pause(self):
        print("Pausing rackmon monitoring...")
        self.rmd.pause()
        time.sleep(RACKMON_SETTLE_SECS)

    def resume(self):
        print("Resuming rackmon monitoring...")
        self.rmd.resume()
