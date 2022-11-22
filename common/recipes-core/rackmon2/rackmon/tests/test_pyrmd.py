import unittest
import asyncio
from unittest.mock import patch

try:
    import pyrmd
except ImportError:
    import sys
    import os

    # Put pyrmd in the correct path relative to source tree.
    sys.path.insert(
        0,
        os.path.join(
            os.path.dirname(os.path.dirname(os.path.realpath(__file__))), "scripts"
        ),
    )
    import pyrmd


@patch("pyrmd.RackmonAsyncInterface._execute", new_callable=unittest.mock.MagicMock)
@patch("pyrmd.RackmonInterface._execute", new_callable=unittest.mock.MagicMock)
class PyrmdSyncTest(unittest.TestCase):
    def setUp(self):
        super().setUp()
        # asyncio.set_event_loop(asyncio.new_event_loop())
        self.loop = asyncio.get_event_loop()

    def do_cmd(
        self,
        sync_exec,
        async_exec,
        exp_req,
        exp_resp,
        exp_ret,
        sync_func,
        async_func,
        *args,
        **kwargs
    ):
        sync_exec.return_value = exp_resp
        async_exec.return_value = asyncio.Future()
        async_exec.return_value.set_result(exp_resp)
        ret = self.loop.run_until_complete(async_func(*args, **kwargs))
        self.assertEqual(ret, exp_ret)
        async_exec.assert_called_with(exp_req)
        ret = sync_func(*args, *kwargs)
        self.assertEqual(ret, exp_ret)
        sync_exec.assert_called_with(exp_req)

    def do_cmd_raises(
        self,
        sync_exec,
        async_exec,
        exp_resp,
        exp_except,
        exp_msg,
        sync_func,
        async_func,
        *args,
        **kwargs
    ):
        sync_exec.return_value = exp_resp
        async_exec.return_value = asyncio.Future()
        async_exec.return_value.set_result(exp_resp)
        if exp_msg is None:
            self.assertRaises(exp_except, sync_func, *args, **kwargs)
            fut = async_func(*args, **kwargs)
            self.assertRaises(exp_except, self.loop.run_until_complete, fut)
        else:
            self.assertRaisesRegex(exp_except, exp_msg, sync_func, *args, **kwargs)
            fut = async_func(*args, **kwargs)
            self.assertRaisesRegex(
                exp_except, exp_msg, self.loop.run_until_complete, fut
            )

    def test_raw_basic(self, sync_exec, async_exec):
        exp_resp = {
            "status": "SUCCESS",
            "data": [0xA4, 0x03, 0x2, 0x12, 0x34, 0xC1, 0xC2],
        }
        exp_ret = b"\xa4\x03\x02\x12\x34"
        exp_req = {
            "type": "raw",
            "response_length": 7,
            "timeout": 0,
            "cmd": [0xA4, 0x03, 0x0, 0x0, 0x0, 0x1],
        }
        self.do_cmd(
            sync_exec,
            async_exec,
            exp_req,
            exp_resp,
            exp_ret,
            pyrmd.RackmonInterface.raw,
            pyrmd.RackmonAsyncInterface.raw,
            b"\xa4\x03\x00\x00\x00\x01",
            7,
        )

    def test_modbuscmd_sync_timeout(self, sync_exec, async_exec):
        exp_resp = {
            "status": "SUCCESS",
            "data": [0xA4, 0x03, 0x2, 0x12, 0x34, 0xC1, 0xC2],
        }
        exp_req = {
            "type": "raw",
            "response_length": 7,
            "timeout": 42,
            "cmd": [0xA4, 0x03, 0x0, 0x0, 0x0, 0x1],
        }
        exp_ret = b"\xa4\x03\x02\x12\x34"
        self.do_cmd(
            sync_exec,
            async_exec,
            exp_req,
            exp_resp,
            exp_ret,
            pyrmd.RackmonInterface.raw,
            pyrmd.RackmonAsyncInterface.raw,
            b"\xa4\x03\x00\x00\x00\x01",
            7,
            42,
        )

    def test_modbuscmd_sync_except_timeout(self, sync_exec, async_exec):
        exp_resp = {"status": "ERR_TIMEOUT"}
        self.do_cmd_raises(
            sync_exec,
            async_exec,
            exp_resp,
            pyrmd.ModbusTimeout,
            None,
            pyrmd.RackmonInterface.raw,
            pyrmd.RackmonAsyncInterface.raw,
            b"\xa4\x03\x00\x00\x00\x01",
            7,
        )

    def test_modbuscmd_sync_except_crc(self, sync_exec, async_exec):
        exp_resp = {"status": "ERR_BAD_CRC"}
        self.do_cmd_raises(
            sync_exec,
            async_exec,
            exp_resp,
            pyrmd.ModbusCRCError,
            None,
            pyrmd.RackmonInterface.raw,
            pyrmd.RackmonAsyncInterface.raw,
            b"\xa4\x03\x00\x00\x00\x01",
            7,
        )

    def test_modbuscmd_sync_except_misc(self, sync_exec, async_exec):
        exp_resp = {"status": "MISC_ERROR"}
        self.do_cmd_raises(
            sync_exec,
            async_exec,
            exp_resp,
            pyrmd.ModbusException,
            "MISC_ERROR",
            pyrmd.RackmonInterface.raw,
            pyrmd.RackmonAsyncInterface.raw,
            b"\xa4\x03\x00\x00\x00\x01",
            7,
        )

    def test_read_register_sync_basic(self, sync_exec, async_exec):
        exp_resp = {"status": "SUCCESS", "regValues": [0x1234]}
        exp_req = {
            "type": "readHoldingRegisters",
            "devAddress": 0xA4,
            "regAddress": 0x0,
            "numRegisters": 0x1,
            "timeout": 0,
        }
        exp_ret = [0x1234]
        self.do_cmd(
            sync_exec,
            async_exec,
            exp_req,
            exp_resp,
            exp_ret,
            pyrmd.RackmonInterface.read,
            pyrmd.RackmonAsyncInterface.read,
            0xA4,
            0x0,
        )

    def test_write_register_sync_single(self, sync_exec, async_exec):
        exp_resp = {"status": "SUCCESS"}
        exp_req = {
            "type": "writeSingleRegister",
            "devAddress": 0xA4,
            "regAddress": 0x0,
            "regValue": 0x1234,
            "timeout": 0,
        }
        self.do_cmd(
            sync_exec,
            async_exec,
            exp_req,
            exp_resp,
            None,
            pyrmd.RackmonInterface.write,
            pyrmd.RackmonAsyncInterface.write,
            0xA4,
            0x0,
            0x1234,
        )

    def test_write_register_sync_multiple(self, sync_exec, async_exec):
        exp_resp = {"status": "SUCCESS"}
        exp_req = {
            "type": "presetMultipleRegisters",
            "devAddress": 0xA4,
            "regAddress": 0x0,
            "regValue": [0x1234, 0x5678],
            "timeout": 0,
        }
        self.do_cmd(
            sync_exec,
            async_exec,
            exp_req,
            exp_resp,
            None,
            pyrmd.RackmonInterface.write,
            pyrmd.RackmonAsyncInterface.write,
            0xA4,
            0x0,
            [0x1234, 0x5678],
        )

    def test_pause_monitoring_sync(self, sync_exec, async_exec):
        exp_resp = {"status": "SUCCESS"}
        exp_req = {"type": "pause"}
        self.do_cmd(
            sync_exec,
            async_exec,
            exp_req,
            exp_resp,
            None,
            pyrmd.RackmonInterface.pause,
            pyrmd.RackmonAsyncInterface.pause,
        )

    def test_resume_monitoring_sync(self, sync_exec, async_exec):
        exp_resp = {"status": "SUCCESS"}
        exp_req = {"type": "resume"}
        self.do_cmd(
            sync_exec,
            async_exec,
            exp_req,
            exp_resp,
            None,
            pyrmd.RackmonInterface.resume,
            pyrmd.RackmonAsyncInterface.resume,
        )

    def test_list_devices_sync(self, sync_exec, async_exec):
        dev1 = {
            "addr": 0xA4,
            "crc_fails": 0,
            "timeouts": 0,
            "misc_fails": 0,
            "mode": "ACTIVE",
            "baudrate": 19200,
            "deviceType": "orv2_psu",
        }
        exp_resp = {"status": "SUCCESS", "data": [dev1]}
        exp_req = {"type": "listModbusDevices"}
        exp_ret = [dev1]
        self.do_cmd(
            sync_exec,
            async_exec,
            exp_req,
            exp_resp,
            exp_ret,
            pyrmd.RackmonInterface.list,
            pyrmd.RackmonAsyncInterface.list,
        )

    def test_monitor_raw_data_sync(self, sync_exec, async_exec):
        exp_resp = {"status": "SUCCESS", "data": []}
        exp_req = {"type": "getMonitorDataRaw"}
        exp_ret = []
        self.do_cmd(
            sync_exec,
            async_exec,
            exp_req,
            exp_resp,
            exp_ret,
            pyrmd.RackmonInterface.data,
            pyrmd.RackmonAsyncInterface.data,
        )

    def test_monitor_data_sync(self, sync_exec, async_exec):
        exp_resp = {"status": "SUCCESS", "data": []}
        exp_req = {"type": "getMonitorData"}
        exp_ret = []
        self.do_cmd(
            sync_exec,
            async_exec,
            exp_req,
            exp_resp,
            exp_ret,
            pyrmd.RackmonInterface.data,
            pyrmd.RackmonAsyncInterface.data,
            False,
        )


if __name__ == "__main__":
    unittest.main()
