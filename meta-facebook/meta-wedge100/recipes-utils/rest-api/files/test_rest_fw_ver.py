import asyncio
import unittest
import uuid

from aiohttp.test_utils import unittest_run_loop
from rest_fw_ver import kv_cache_async


class TestKvCache(unittest.TestCase):
    def setUp(self):
        self.loop = asyncio.new_event_loop()
        self.called = False

        @kv_cache_async(str(uuid.uuid1()))
        async def fake_cached_func():
            if not self.called:
                self.called = True
                return "fake_expensive_result"
            else:
                raise Exception("This should have been cached")

        self.f = fake_cached_func

    @unittest_run_loop
    async def test_kv_cache(self):
        self.assertEqual("fake_expensive_result", await self.f())
        self.assertTrue(self.called)
        self.assertEqual("fake_expensive_result", await self.f())
