import os
import tempfile
import unittest
import unittest.mock

import rest_logfile
from aiohttp import web
from aiohttp.test_utils import AioHTTPTestCase, unittest_run_loop


class TestRestLogfile(unittest.TestCase):
    def setUp(self):
        self.tmpdir = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmpdir.cleanup)

        self.logfile = os.path.join(self.tmpdir.name, "logfile")
        self.rotated = self.logfile + ".0"

        patch = unittest.mock.patch.multiple(
            rest_logfile, LOGFILE=self.logfile, ROTATED_LOGFILE=self.rotated
        )
        patch.start()
        self.addCleanup(patch.stop)

    def _write(self, path, lines):
        with open(path, "w") as f:
            f.write("\n".join(lines) + "\n")

    def test_missing_logfile_is_not_an_error(self):
        result = rest_logfile.get_logfile()
        self.assertEqual(result["Information"]["entries"], [])
        self.assertEqual(result["Information"]["count"], 0)
        self.assertEqual(result["Information"]["sources"], [])

    def test_returns_entries_and_source(self):
        self._write(self.logfile, ["line one", "line two"])
        result = rest_logfile.get_logfile()
        self.assertEqual(result["Information"]["entries"], ["line one", "line two"])
        self.assertEqual(result["Information"]["count"], 2)
        self.assertFalse(result["Information"]["truncated"])
        self.assertEqual(result["Information"]["sources"], [self.logfile])

    def test_tail_keeps_the_newest_lines(self):
        self._write(self.logfile, ["l%d" % i for i in range(10)])
        result = rest_logfile.get_logfile(lines=3)
        self.assertEqual(result["Information"]["entries"], ["l7", "l8", "l9"])
        self.assertTrue(result["Information"]["truncated"])

    def test_rotated_is_excluded_by_default(self):
        self._write(self.rotated, ["old"])
        self._write(self.logfile, ["new"])
        result = rest_logfile.get_logfile()
        self.assertEqual(result["Information"]["entries"], ["new"])
        self.assertEqual(result["Information"]["sources"], [self.logfile])

    def test_rotated_is_prepended_when_requested(self):
        # The rotated generation holds the older half of the history, so it has
        # to come first for the tail to mean what callers expect.
        self._write(self.rotated, ["old"])
        self._write(self.logfile, ["new"])
        result = rest_logfile.get_logfile(include_rotated=True)
        self.assertEqual(result["Information"]["entries"], ["old", "new"])
        self.assertEqual(result["Information"]["sources"], [self.rotated, self.logfile])

    def test_undecodable_bytes_do_not_raise(self):
        with open(self.logfile, "wb") as f:
            f.write(b"good\n\xff\xfe bad\n")
        result = rest_logfile.get_logfile()
        self.assertEqual(result["Information"]["count"], 2)


class TestPostLogfile(AioHTTPTestCase):
    async def get_application(self):
        app = web.Application()
        app.router.add_post("/api/sys/logfile", rest_logfile.post_logfile)
        return app

    def setUp(self):
        super().setUp()
        self.tmpdir = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmpdir.cleanup)
        logfile = os.path.join(self.tmpdir.name, "logfile")
        with open(logfile, "w") as f:
            f.write("\n".join("l%d" % i for i in range(500)) + "\n")
        patch = unittest.mock.patch.multiple(
            rest_logfile, LOGFILE=logfile, ROTATED_LOGFILE=logfile + ".0"
        )
        patch.start()
        self.addCleanup(patch.stop)

    async def _post(self, payload=None):
        if payload is None:
            return await self.client.request("POST", "/api/sys/logfile")
        return await self.client.request("POST", "/api/sys/logfile", json=payload)

    @unittest_run_loop
    async def test_empty_body_uses_defaults(self):
        resp = await self._post()
        self.assertEqual(resp.status, 200)
        body = await resp.json()
        self.assertEqual(body["Information"]["count"], rest_logfile.DEFAULT_LINES)

    @unittest_run_loop
    async def test_lines_is_honoured(self):
        resp = await self._post({"lines": 3})
        self.assertEqual(resp.status, 200)
        self.assertEqual((await resp.json())["Information"]["count"], 3)

    @unittest_run_loop
    async def test_lines_is_clamped_to_max(self):
        resp = await self._post({"lines": 99999})
        self.assertEqual(
            (await resp.json())["Information"]["count"], rest_logfile.MAX_LINES
        )

    @unittest_run_loop
    async def test_lines_is_clamped_to_one(self):
        for value in (0, -5):
            resp = await self._post({"lines": value})
            self.assertEqual((await resp.json())["Information"]["count"], 1)

    @unittest_run_loop
    async def test_include_rotated_adds_the_rotated_source(self):
        with open(rest_logfile.ROTATED_LOGFILE, "w") as f:
            f.write("old\n")
        resp = await self._post({"lines": 5, "include_rotated": True})
        sources = (await resp.json())["Information"]["sources"]
        self.assertEqual(sources, [rest_logfile.ROTATED_LOGFILE, rest_logfile.LOGFILE])

    @unittest_run_loop
    async def test_non_integer_lines_is_rejected(self):
        resp = await self._post({"lines": "abc"})
        self.assertEqual(resp.status, 400)
        self.assertEqual(
            (await resp.json())["Information"]["reason"], "lines must be an integer"
        )

    @unittest_run_loop
    async def test_boolean_lines_is_rejected(self):
        # bool is a subclass of int; True must not be read as lines=1.
        resp = await self._post({"lines": True})
        self.assertEqual(resp.status, 400)

    @unittest_run_loop
    async def test_non_boolean_include_rotated_is_rejected(self):
        resp = await self._post({"include_rotated": "yes"})
        self.assertEqual(resp.status, 400)

    @unittest_run_loop
    async def test_non_object_body_is_rejected(self):
        resp = await self._post([1, 2, 3])
        self.assertEqual(resp.status, 400)

    @unittest_run_loop
    async def test_unparseable_body_is_rejected(self):
        resp = await self.client.request(
            "POST",
            "/api/sys/logfile",
            data="garbage",
            headers={"Content-Type": "application/json"},
        )
        self.assertEqual(resp.status, 400)


if __name__ == "__main__":
    unittest.main()
