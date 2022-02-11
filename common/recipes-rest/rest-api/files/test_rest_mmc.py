import contextlib
import ctypes
import unittest

import aiohttp
import obmc_mmc
import rest_mmc
from aiohttp.test_utils import AioHTTPTestCase, unittest_run_loop

EXAMPLE_CID = b"\x11\x01\x00A1234B\x00\x004\x97\xe6\x01\x00\x99\x00\x00\x00"


class TestRestMMC(AioHTTPTestCase):
    def setUp(self):
        super().setUp()

        self.patches = [
            unittest.mock.patch(
                "obmc_mmc.list_devices",
                autospec=True,
                return_value=["/dev/mmcblk123"],
            ),
            unittest.mock.patch(
                "obmc_mmc.mmc_dev",
                autospec=True,
                return_value=nullcontext(),
            ),
            unittest.mock.patch(
                "obmc_mmc.mmc_cid_read",
                autospec=True,
                return_value=obmc_mmc.mmc_cid_t.from_buffer_copy(EXAMPLE_CID),
            ),
            unittest.mock.patch(
                "obmc_mmc.mmc_extcsd_read",
                autospec=True,
                return_value=obmc_mmc.mmc_extcsd_t(raw=(ctypes.c_uint8 * 512)()),
            ),
            unittest.mock.patch(
                "obmc_mmc.extcsd_life_time_estimate",
                autospec=True,
                return_value=obmc_mmc.ExtCsdLifeTimeEstimate(
                    EXT_CSD_PRE_EOL_INFO=1,
                    EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A=2,
                    EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B=3,
                ),
            ),
        ]
        for p in self.patches:
            p.start()
            self.addCleanup(p.stop)

    def test_list_mmc_devices(self):
        ret = rest_mmc._list_mmc_devices()
        self.assertEqual(ret, ["/dev/mmcblk123"])

    def test_get_dev_info(self):
        ret = rest_mmc._get_dev_info("<dev_path>")
        self.assertEqual(
            ret,
            {
                "CID_MID": 17,
                "CID_PNM": "A1234B",
                "CID_PRV_MAJOR": 0,
                "CID_PRV_MINOR": 52,
                "EXT_CSD_PRE_EOL_INFO": 1,
                "EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A": 2,
                "EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B": 3,
            },
        )

        obmc_mmc.mmc_dev.assert_called_once_with("<dev_path>")
        obmc_mmc.extcsd_life_time_estimate.assert_called_once_with(
            obmc_mmc.mmc_extcsd_read.return_value
        )

    def test_get_dev_info_suppress_exceptions(self):
        obmc_mmc.mmc_dev.side_effect = obmc_mmc.LibObmcMmcException("blah")

        # Should not raise exception
        with self.assertLogs() as cm:
            ret = rest_mmc._get_dev_info("<dev_path>")

        self.assertEqual(ret, {})

        # Ensure exception was logged
        self.assertRegex(
            cm.records[0].msg,
            r"Error while attempting to fetch mmc health data for '<dev_path>': LibObmcMmcException.*blah.*[)]",  # noqa: B950
        )

    @unittest_run_loop
    async def test_get_mmc_info(self):
        ret = await rest_mmc.get_mmc_info()

        self.assertEqual(
            ret,
            {
                "/dev/mmcblk123": {
                    "CID_MID": 17,
                    "CID_PNM": "A1234B",
                    "CID_PRV_MAJOR": 0,
                    "CID_PRV_MINOR": 52,
                    "EXT_CSD_PRE_EOL_INFO": 1,
                    "EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A": 2,
                    "EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B": 3,
                }
            },
        )

    async def get_application(self):
        return aiohttp.web.Application()


# python 3.5 doesn't have contextlib.nullcontext
@contextlib.contextmanager
def nullcontext():
    yield unittest.mock.Mock()
