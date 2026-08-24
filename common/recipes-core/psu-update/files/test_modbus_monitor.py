import io
import unittest
from unittest.mock import call, MagicMock, patch

import modbus_monitor
import test_mocks  # noqa: F401  installs the fake pyrmd/minimalmodbus
from modbus_common import ModbusException, PMM_PAUSE_REG
from modbus_monitor import (
    Monitor,
    MonitorChain,
    NullMonitor,
    PHOSPHOR_MODBUS_SETTLE_SECS,
    PhosphorModbusMonitor,
    PMM_SETTLE_SECS,
    PmmMonitor,
    RACKMON_SETTLE_SECS,
    RackmonMonitor,
)


class RecordingMonitor(Monitor):
    def __init__(self, log, name, fail_on_pause=False):
        self.log = log
        self.name = name
        self.fail_on_pause = fail_on_pause

    def pause(self):
        self.log.append("pause " + self.name)
        if self.fail_on_pause:
            raise ModbusException("cannot pause " + self.name)

    def resume(self):
        self.log.append("resume " + self.name)


class NoiseFree(unittest.TestCase):
    """Swallows the progress messages the monitors print"""

    def setUp(self):
        patcher = patch("sys.stdout", new=io.StringIO())
        self.stdout = patcher.start()
        self.addCleanup(patcher.stop)
        patcher = patch.object(modbus_monitor.time, "sleep")
        self.sleep = patcher.start()
        self.addCleanup(patcher.stop)


class TestMonitor(NoiseFree):
    def test_suppress_pauses_and_resumes(self):
        log = []
        with RecordingMonitor(log, "a").suppress():
            log.append("update")
        self.assertEqual(log, ["pause a", "update", "resume a"])

    def test_a_failed_update_still_resumes_monitoring(self):
        log = []
        with self.assertRaises(ValueError):
            with RecordingMonitor(log, "a").suppress():
                raise ValueError("update failed")
        self.assertEqual(log, ["pause a", "resume a"])

    def test_the_base_monitor_does_nothing(self):
        with Monitor().suppress():
            pass

    def test_null_monitor_does_nothing(self):
        with NullMonitor().suppress():
            pass


class TestMonitorChain(NoiseFree):
    def test_suppresses_in_order_and_resumes_in_reverse(self):
        log = []
        chain = MonitorChain(RecordingMonitor(log, "a"), RecordingMonitor(log, "b"))
        with chain.suppress():
            log.append("update")
        self.assertEqual(log, ["pause a", "pause b", "update", "resume b", "resume a"])

    def test_nones_are_dropped(self):
        log = []
        chain = MonitorChain(None, RecordingMonitor(log, "a"), None)
        self.assertEqual(len(chain.monitors), 1)
        with chain.suppress():
            pass
        self.assertEqual(log, ["pause a", "resume a"])

    def test_an_empty_chain_is_usable(self):
        with MonitorChain().suppress():
            pass

    def test_a_monitor_which_cannot_be_paused_aborts_the_update(self):
        # And unwinds the monitors already suppressed, without touching
        # the ones after it.
        log = []
        chain = MonitorChain(
            RecordingMonitor(log, "a"),
            RecordingMonitor(log, "b", fail_on_pause=True),
            RecordingMonitor(log, "c"),
        )
        with self.assertRaises(ModbusException):
            with chain.suppress():
                log.append("update")
        self.assertEqual(log, ["pause a", "pause b", "resume a"])


class TestPmmMonitor(NoiseFree):
    def monitor(self):
        pmm = MagicMock()
        pmm.dev_addr = 0x15
        return PmmMonitor(pmm), pmm

    def test_pause_and_resume_drive_the_pause_register(self):
        monitor, pmm = self.monitor()
        with monitor.suppress():
            pmm.write.assert_called_once_with(PMM_PAUSE_REG, 0x1)
        self.assertEqual(
            pmm.write.call_args_list,
            [call(PMM_PAUSE_REG, 0x1), call(PMM_PAUSE_REG, 0x0)],
        )

    def test_pause_gives_the_pmm_time_to_stand_off(self):
        monitor, _ = self.monitor()
        monitor.pause()
        self.sleep.assert_called_once_with(PMM_SETTLE_SECS)

    def test_a_pmm_which_will_not_pause_only_warns(self):
        # It degrades the update, it does not corrupt it.
        monitor, pmm = self.monitor()
        pmm.write.side_effect = ModbusException("ERR_TIMEOUT")
        with monitor.suppress():
            pass
        self.assertIn("WARNING: Control PMM:21 1 failed", self.stdout.getvalue())
        self.assertIn("WARNING: Control PMM:21 0 failed", self.stdout.getvalue())


class TestRackmonMonitor(NoiseFree):
    def test_pause_and_resume_go_through_rackmon(self):
        monitor = RackmonMonitor()
        with patch.object(monitor, "rmd") as rmd:
            with monitor.suppress():
                rmd.pause.assert_called_once_with()
                rmd.resume.assert_not_called()
            rmd.resume.assert_called_once_with()

    def test_pause_waits_for_the_polling_threads_to_exit(self):
        # Driving the bus while rackmon still has it corrupts the transfer.
        monitor = RackmonMonitor()
        with patch.object(monitor, "rmd"):
            monitor.pause()
        self.sleep.assert_called_once_with(RACKMON_SETTLE_SECS)

    def test_resume_hands_the_bus_straight_back(self):
        # Nothing of ours runs after it, so there is nothing to wait for.
        monitor = RackmonMonitor()
        with patch.object(monitor, "rmd"):
            monitor.resume()
        self.sleep.assert_not_called()


class TestPhosphorModbusMonitor(NoiseFree):
    PORT = "/dev/ttyRS485-1"

    def setUp(self):
        super().setUp()
        patcher = patch.object(
            modbus_monitor.phosphor_modbus, "PhosphorModbusExclusion"
        )
        self.exclusion_cls = patcher.start()
        self.addCleanup(patcher.stop)
        self.exclusion = self.exclusion_cls.return_value
        self.exclusion.get_port_paths.return_value = ["/xyz/port/ttyRS485_1"]
        patcher = patch.object(
            modbus_monitor.phosphor_modbus, "is_unit_running", return_value=True
        )
        self.is_unit_running = patcher.start()
        self.addCleanup(patcher.stop)

    def test_suppress_stops_and_restarts_the_exclusion(self):
        self.exclusion.stop.return_value = True
        monitor = PhosphorModbusMonitor(self.PORT)
        with monitor.suppress():
            self.exclusion.stop.assert_called_once_with()
            self.exclusion.start.assert_not_called()
        self.exclusion.start.assert_called_once_with()

    def test_pause_waits_for_the_daemon_to_let_go(self):
        self.exclusion.stop.return_value = True
        PhosphorModbusMonitor(self.PORT).pause()
        self.sleep.assert_called_once_with(PHOSPHOR_MODBUS_SETTLE_SECS)

    def test_a_port_the_service_does_not_manage_is_refused(self):
        # Something else is polling it; driving it would corrupt the
        # transfer.
        self.exclusion.get_port_paths.return_value = []
        with self.assertRaises(ValueError):
            PhosphorModbusMonitor(self.PORT)

    def test_an_unenumerable_port_is_treated_as_unmanaged(self):
        self.exclusion.get_port_paths.side_effect = (
            modbus_monitor.phosphor_modbus.ConfigError("no bus")
        )
        with self.assertRaises(ValueError):
            PhosphorModbusMonitor(self.PORT)

    def test_a_stopped_service_leaves_the_port_unsupervised(self):
        self.is_unit_running.return_value = False
        monitor = PhosphorModbusMonitor(self.PORT)
        self.assertIn("is not running", self.stdout.getvalue())
        # Nothing to exclude, so a stop() which reports False is fine.
        self.exclusion.stop.return_value = False
        with monitor.suppress():
            pass
        self.exclusion.start.assert_called_once_with()

    def test_a_service_which_will_not_stop_polling_aborts_the_update(self):
        monitor = PhosphorModbusMonitor(self.PORT)
        self.exclusion.stop.return_value = False
        with self.assertRaises(ModbusException):
            with monitor.suppress():
                self.fail("the update must not run")

    def test_port_is_managed_is_false_when_the_service_is_down(self):
        monitor = PhosphorModbusMonitor(self.PORT)
        self.is_unit_running.return_value = False
        self.assertFalse(monitor.port_is_managed())

    def test_the_exclusion_covers_the_port_it_was_asked_for(self):
        PhosphorModbusMonitor(self.PORT)
        self.exclusion_cls.assert_called_once_with(self.PORT)


if __name__ == "__main__":
    unittest.main()
