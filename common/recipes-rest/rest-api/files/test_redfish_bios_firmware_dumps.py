import asyncio
import logging
import os
import unittest
import unittest.mock
from contextlib import contextmanager
from typing import List

import aiohttp.web
import redfish_bios_firmware_dumps
import test_mock_modules  # noqa: F401
from aiohttp.test_utils import AioHTTPTestCase, unittest_run_loop
from common_middlewares import jsonerrorhandler
from test_redfish_computer_system_patches import patches


def new_stat_result(**kwargs) -> os.stat_result:
    result = unittest.mock.MagicMock()
    for key, value in kwargs.items():
        setattr(result, key, value)
    return result


class MockFile:
    def __init__(self, read_data="", stat=None, exists=True):
        if stat is None:
            stat = new_stat_result()
        self.mock_open = unittest.mock.mock_open(read_data=read_data)
        self.stat = stat
        self.exists = exists

    mock_open: unittest.mock.MagicMock
    stat: os.stat_result
    exists: bool


class MockRepo:
    _repo_path: str

    def __init__(self, repo_path: str):
        if repo_path[-1] != "/":
            repo_path = "{}/".format(repo_path)

        self._repo_path = repo_path

    file_map = {
        "server1": MockFile(),
        "server1/dump_1": MockFile(stat=new_stat_result(st_atime=1)),
        "server1/dump_1/exitcode": MockFile(read_data="0\n"),
        "server1/dump_1/log": MockFile(
            read_data="debug log entry 0\ndebug log entry 1\n"
        ),
        "server1/dump_1/err": MockFile(read_data="err log entry 0\nerr log entry 1"),
        "server1/dump_1/image": MockFile(
            read_data="some image", stat=new_stat_result(st_size=2, st_ctime=3)
        ),
        "server1/dump_1/fwutil-pid": MockFile(read_data="1111"),
        "server1/dump_2": MockFile(stat=new_stat_result(st_atime=4)),
        "server1/dump_2/exitcode": MockFile(read_data="1"),
        "server1/dump_2/err": MockFile(read_data="err log entry 0\nerr log entry 1"),
        "server1/dump_2/image": MockFile(exists=False),
        "server1/dump_3": MockFile(stat=new_stat_result(st_atime=5)),
        "server1/dump_3/exitcode": MockFile(read_data="\n2\n"),
        "server1/dump_3/image": MockFile(exists=False),
        "server1/dump_4": MockFile(stat=new_stat_result(st_atime=6)),
        "server1/dump_4/image": MockFile(
            read_data="some image", stat=new_stat_result(st_size=7, st_ctime=8)
        ),
        "server1/dump_4/exitcode": MockFile(exists=False),
        "server1/dump_4/fwutil-pid": MockFile(exists=False),
        "server1/my_dump": MockFile(exists=False),
        "server1/my_dump/exitcode": MockFile(exists=False),
        "server1/my_dump/fwutil-pid": MockFile(exists=False),
        "server1/my_dump/image": MockFile(exists=False),
        "server1/x": MockFile(exists=False),
        "server1/non_existent": MockFile(exists=False),
    }

    @contextmanager
    def do_mock_filesystem(self):
        os_stat_func_orig = os.stat

        def mock_os_stat(*args, **kwargs) -> os.stat_result:
            return self._mock_os_stat(os_stat_func_orig, *args, **kwargs)

        def mock_os_path_exists(*args, **kwargs) -> bool:
            return self._mock_os_path_exists(*args, **kwargs)

        def mock_os_listdir(*args, **kwargs) -> List[str]:
            return self._mock_os_listdir(*args, **kwargs)

        def mock_os_mkdir(*args, **kwargs):
            return self._mock_os_mkdir(*args, **kwargs)

        def mock_builtins_open(*args, **kwargs):
            return self._mock_builtins_open(*args, **kwargs)

        with unittest.mock.patch("os.stat", mock_os_stat):
            with unittest.mock.patch("os.path.exists", mock_os_path_exists):
                with unittest.mock.patch("os.listdir", mock_os_listdir):
                    with unittest.mock.patch("os.mkdir", mock_os_mkdir):
                        with unittest.mock.patch("builtins.open", mock_builtins_open):
                            yield

    def _mock_os_stat(self, os_stat_func_orig, *args, **kwargs) -> os.stat_result:
        path = args[0]
        if path.find(self._repo_path) != 0:
            return os_stat_func_orig(*args, **kwargs)
        path_rel = path[len(self._repo_path) : len(path)]
        if path_rel not in self.file_map:
            raise Exception(
                "os.stat: unexpected path: '{}' ('{}')".format(path_rel, path)
            )
        file = self.file_map[path_rel]
        return file.stat

    def _mock_os_path_exists(self, path: str) -> bool:
        if path.find(self._repo_path) != 0:
            raise Exception("os.path.exists: unexpected root in path '{}'".format(path))
        path_rel = path[len(self._repo_path) : len(path)]
        if path_rel not in self.file_map:
            raise Exception(
                "os.path.exists: unexpected path: '{}' ('{}')".format(path_rel, path)
            )
        file = self.file_map[path_rel]
        return file.exists

    def _mock_os_listdir(self, dir_abs: str) -> List[str]:
        if dir_abs.find(self._repo_path) != 0:
            raise Exception("os.listdir: unexpected root in path '{}'".format(dir_abs))
        result: List[str] = []
        dir_rel = dir_abs[len(self._repo_path) : len(dir_abs)]
        for path in self.file_map:
            if not self.file_map[path].exists:
                continue
            if path.find(dir_rel + "/") != 0:
                continue
            item_rel = path[len(dir_rel + "/") : len(path)]
            if item_rel.find("/") >= 0:
                continue
            result.append(item_rel)
        sorted(result)
        return result

    def _mock_os_mkdir(self, dir_abs: str):
        if dir_abs.find(self._repo_path) != 0:
            raise Exception("os.mkdir: unexpected root in '{}'".format(dir_abs))
        dir_rel = dir_abs[len(self._repo_path) : len(dir_abs)]
        if dir_rel not in self.file_map:
            raise Exception(
                "os.mkdir: unexpected path: '{}' ('{}')".format(dir_rel, dir_abs)
            )
        _dir = self.file_map[dir_rel]
        if _dir.exists:
            raise FileExistsError("os.mkdir: directory '{}' already exists", dir_abs)
        _dir.exists = True

    def _mock_builtins_open(self, path: str, mode=None):
        if path.find(self._repo_path) != 0:
            raise Exception("open: unexpected path: '{}'".format(path))
        path_rel = path[len(self._repo_path) : len(path)]
        if path_rel not in self.file_map:
            raise Exception("open: unexpected path: '{}' ('{}')".format(path_rel, path))
        file = self.file_map[path_rel]
        if mode is None and not file.exists:
            raise FileNotFoundError(
                "open: file '{}' ('{}') does not exist".format(path_rel, path)
            )
        file.exists = True
        return file.mock_open(path, mode)


class TestBIOSFirmwareDumps(AioHTTPTestCase):
    _repo_path = "/tmp/RedfishBIOSFirmwareDumps-mockpath"

    async def setUpAsync(self):
        asyncio.set_event_loop(asyncio.new_event_loop())
        self.patches = patches()
        self.patches.append(
            unittest.mock.patch(
                "rest_pal_legacy.pal_get_num_slots",
                create=True,
                return_value=4,
            ),
        )
        for p in self.patches:
            p.start()
            self.addCleanup(p.stop)

        if redfish_bios_firmware_dumps.has_psutil:
            redfish_bios_firmware_dumps.has_psutil = False

            def redfish_bios_firmware_dumps_cleanup():
                redfish_bios_firmware_dumps.has_psutil = True

            self.addCleanup(redfish_bios_firmware_dumps_cleanup)

        await super().setUpAsync()

    @contextmanager
    def mock_repo(self):
        repo = MockRepo(self._repo_path)
        with repo.do_mock_filesystem():
            yield repo

    @unittest_run_loop
    async def test_get_collection_descriptor(self):
        expected_resp = {
            "@odata.id": "/redfish/v1/Systems/server1/Bios/FirmwareDumps",
            "@odata.type": "#FirmwareDumps.v1_0_0.FirmwareDumps",
            "Id": "server1 BIOS dumps",
            "Name": "server1 BIOS dumps",
            "Members@odata.count": 5,
            "Members": [
                {
                    "@odata.id": "/redfish/v1/Systems/server1/Bios/FirmwareDumps/dump_1",
                    "@odata.type": "#BIOSFirmwareDump.v1_0_0.BIOSFirmwareDump",
                    "Actions": {
                        "#BIOSFirmwareDump.ReadContent": {
                            "target": "/redfish/v1/Systems/server1/Bios/FirmwareDumps/dump_1/Actions/BIOSFirmwareDump.ReadContent"
                        }
                    },
                    "Id": "dump_1",
                    "Log": {
                        "Debug": "debug log entry 0\ndebug log entry 1\n",
                        "Error": "err log entry 0\nerr log entry 1",
                        "ExitCode": 0,
                    },
                    "Name": "BIOS firmware dump",
                    "SizeCurrent": 2,
                    "SizeTotal": 2,
                    "Status": "success",
                    "TimestampEndMS": 3000,
                    "TimestampStartMS": 1000,
                },
                {
                    "@odata.id": "/redfish/v1/Systems/server1/Bios/FirmwareDumps/dump_2",
                    "@odata.type": "#BIOSFirmwareDump.v1_0_0.BIOSFirmwareDump",
                    "Actions": {
                        "#BIOSFirmwareDump.ReadContent": {
                            "target": "/redfish/v1/Systems/server1/Bios/FirmwareDumps/dump_2/Actions/BIOSFirmwareDump.ReadContent"
                        }
                    },
                    "Id": "dump_2",
                    "Log": {"Error": "err log entry 0\nerr log entry 1", "ExitCode": 1},
                    "Name": "BIOS firmware dump",
                    "Status": "failure",
                },
                {
                    "@odata.id": "/redfish/v1/Systems/server1/Bios/FirmwareDumps/dump_3",
                    "@odata.type": "#BIOSFirmwareDump.v1_0_0.BIOSFirmwareDump",
                    "Actions": {
                        "#BIOSFirmwareDump.ReadContent": {
                            "target": "/redfish/v1/Systems/server1/Bios/FirmwareDumps/dump_3/Actions/BIOSFirmwareDump.ReadContent"
                        }
                    },
                    "Id": "dump_3",
                    "Log": {"ExitCode": 2},
                    "Name": "BIOS firmware dump",
                    "Status": "failure",
                },
                {
                    "@odata.id": "/redfish/v1/Systems/server1/Bios/FirmwareDumps/dump_4",
                    "@odata.type": "#BIOSFirmwareDump.v1_0_0.BIOSFirmwareDump",
                    "Actions": {
                        "#BIOSFirmwareDump.ReadContent": {
                            "target": "/redfish/v1/Systems/server1/Bios/FirmwareDumps/dump_4/Actions/BIOSFirmwareDump.ReadContent"
                        }
                    },
                    "Id": "dump_4",
                    "Log": {},
                    "Name": "BIOS firmware dump",
                    "SizeCurrent": 7,
                    "Status": "unknown",
                    "TimestampStartMS": 6000,
                },
                {
                    "@odata.id": "/redfish/v1/Systems/server1/Bios/FirmwareDumps/my_dump",
                    "@odata.type": "#BIOSFirmwareDump.v1_0_0.BIOSFirmwareDump",
                    "Actions": {
                        "#BIOSFirmwareDump.ReadContent": {
                            "target": "/redfish/v1/Systems/server1/Bios/FirmwareDumps/my_dump/Actions/BIOSFirmwareDump.ReadContent"
                        }
                    },
                    "Id": "my_dump",
                    "Log": {},
                    "Name": "BIOS firmware dump",
                    "Status": "unknown",
                },
            ],
        }

        with self.mock_repo():
            req_success = await self.client.request(
                "GET", "/redfish/v1/Systems/server1/Bios/FirmwareDumps"
            )
            if req_success.status != 200:
                self.assertEqual(
                    req_success.status, 200, msg=await req_success.content.read()
                )
            req_not_found = await self.client.request(
                "GET", "/redfish/v1/Systems/server5/Bios/FirmwareDumps"
            )
            self.assertEqual(
                req_not_found.status, 404, msg=await req_not_found.content.read()
            )

        resp = await req_success.json()
        self.maxDiff = None
        self.assertEqual(resp, expected_resp)

    @unittest_run_loop
    async def test_create_dump(self):
        expected_resp = {
            "@odata.id": "/redfish/v1/Systems/server1/Bios/FirmwareDumps/my_dump",
            "@odata.type": "#BIOSFirmwareDump.v1_0_0.BIOSFirmwareDump",
            "Actions": {
                "#BIOSFirmwareDump.ReadContent": {
                    "target": "/redfish/v1/Systems/server1/"
                    + "Bios/FirmwareDumps/my_dump/Actions/BIOSFirmwareDump.ReadContent",
                },
            },
            "Id": "my_dump",
            "Log": {},
            "Name": "BIOS firmware dump",
            "Status": "unknown",
        }

        with self.mock_repo() as repo:
            with unittest.mock.patch("os.spawnvpe", return_value=1234):
                req_duplicate = await self.client.request(
                    "POST", "/redfish/v1/Systems/server1/Bios/FirmwareDumps/dump_1"
                )
                self.assertEqual(
                    req_duplicate.status, 409, msg=await req_duplicate.content.read()
                )
                req_success = await self.client.request(
                    "POST", "/redfish/v1/Systems/server1/Bios/FirmwareDumps/my_dump"
                )
                if req_success.status != 200:
                    self.assertEqual(
                        req_success.status, 200, msg=await req_success.content.read()
                    )
                req_exceeded_limit_1 = await self.client.request(
                    "POST", "/redfish/v1/Systems/server1/Bios/FirmwareDumps/my_dump2"
                )
                self.assertEqual(
                    req_exceeded_limit_1.status,
                    507,
                    msg=await req_exceeded_limit_1.content.read(),
                )
                req_exceeded_limit_2 = await self.client.request(
                    "POST", "/redfish/v1/Systems/server1/Bios/FirmwareDumps/"
                )
                self.assertEqual(
                    req_exceeded_limit_2.status,
                    507,
                    msg=await req_exceeded_limit_2.content.read(),
                )
            self.assertEqual(repo.file_map["server1/my_dump"].exists, True)
            self.assertEqual(repo.file_map["server1/my_dump/fwutil-pid"].exists, True)
            repo.file_map[
                "server1/my_dump/fwutil-pid"
            ].mock_open().write.assert_called_once_with("1234")

        resp = await req_success.json()
        self.maxDiff = None
        self.assertEqual(resp, expected_resp)

    @unittest_run_loop
    async def test_get_dump_descriptor(self):
        expected_resp = {
            "@odata.id": "/redfish/v1/Systems/server1/Bios/FirmwareDumps/dump_1",
            "@odata.type": "#BIOSFirmwareDump.v1_0_0.BIOSFirmwareDump",
            "Actions": {
                "#BIOSFirmwareDump.ReadContent": {
                    "target": "/redfish/v1/Systems/server1/Bios/FirmwareDumps/dump_1/Actions/BIOSFirmwareDump.ReadContent"
                }
            },
            "Id": "dump_1",
            "Log": {
                "Debug": "debug log entry 0\ndebug log entry 1\n",
                "Error": "err log entry 0\nerr log entry 1",
                "ExitCode": 0,
            },
            "Name": "BIOS firmware dump",
            "SizeCurrent": 2,
            "SizeTotal": 2,
            "Status": "success",
            "TimestampEndMS": 3000,
            "TimestampStartMS": 1000,
        }

        with self.mock_repo():
            req_success = await self.client.request(
                "GET", "/redfish/v1/Systems/server1/Bios/FirmwareDumps/dump_1"
            )
            if req_success.status != 200:
                self.assertEqual(
                    req_success.status, 200, msg=await req_success.content.read()
                )
            req_invalid = await self.client.request(
                "GET", "/redfish/v1/Systems/server1/Bios/FirmwareDumps/dump 1"
            )
            self.assertEqual(
                req_invalid.status, 500, msg=await req_invalid.content.read()
            )
            req_not_found = await self.client.request(
                "GET", "/redfish/v1/Systems/server1/Bios/FirmwareDumps/non_existent"
            )
            self.assertEqual(
                req_not_found.status, 404, msg=await req_not_found.content.read()
            )

        resp = await req_success.json()
        self.maxDiff = None
        self.assertEqual(resp, expected_resp)

    @unittest_run_loop
    async def test_read_dump_content(self):
        with self.mock_repo() as repo:
            req_success_full = await self.client.request(
                "GET",
                "/redfish/v1/Systems/server1/Bios/FirmwareDumps/dump_1/Actions/BIOSFirmwareDump.ReadContent",
            )
            if req_success_full.status != 200:
                self.assertEqual(
                    req_success_full.status,
                    200,
                    msg=await req_success_full.content.read(),
                )
            req_success_onebyte = await self.client.request(
                "GET",
                "/redfish/v1/Systems/server1/Bios/FirmwareDumps/dump_1/Actions/BIOSFirmwareDump.ReadContent"
                + "?start_pos=2&end_pos=3",
            )
            if req_success_onebyte.status != 200:
                self.assertEqual(
                    req_success_onebyte.status,
                    200,
                    msg=await req_success_onebyte.content.read(),
                )
            req_not_found = await self.client.request(
                "GET",
                "/redfish/v1/Systems/server1/Bios/FirmwareDumps/x/Actions/BIOSFirmwareDump.ReadContent",
            )
            self.assertEqual(
                req_not_found.status, 404, msg=await req_not_found.content.read()
            )

            self.maxDiff = None
            self.assertEqual(await req_success_full.content.read(), b"some image")
            self.assertEqual(len(await req_success_onebyte.content.read()), 1)
            repo.file_map[
                "server1/dump_1/image"
            ].mock_open().seek.assert_called_once_with(2)

    @unittest_run_loop
    async def test_delete_dump(self):
        with self.mock_repo():
            req_success = await self.client.request(
                "DELETE", "/redfish/v1/Systems/server1/Bios/FirmwareDumps/dump_1"
            )
            if req_success.status != 200:
                self.assertEqual(
                    req_success.status, 200, msg=await req_success.content.read()
                )
            req_in_progress = await self.client.request(
                "DELETE", "/redfish/v1/Systems/server1/Bios/FirmwareDumps/dump_4"
            )
            self.assertEqual(
                req_in_progress.status, 400, msg=await req_in_progress.content.read()
            )
            req_not_found = await self.client.request(
                "DELETE", "/redfish/v1/Systems/server1/Bios/FirmwareDumps/x"
            )
            self.assertEqual(
                req_not_found.status, 404, msg=await req_not_found.content.read()
            )

    async def get_application(self):
        webapp = aiohttp.web.Application(middlewares=[jsonerrorhandler])
        logging.basicConfig(level=logging.DEBUG)

        with self.mock_repo():
            amount_of_images = len(os.listdir("{}/server1".format(self._repo_path)))

        with unittest.mock.patch("os.mkdir") as os_mkdir_mock:
            with unittest.mock.patch(
                "os.stat", return_value=new_stat_result(st_uid=1000)
            ):
                with unittest.mock.patch("os.getuid", return_value=1000):
                    bios_firmware_dumps = (
                        redfish_bios_firmware_dumps.RedfishBIOSFirmwareDumps(
                            images_dir=self._repo_path,
                            image_count_limit=amount_of_images + 1,
                        )
                    )
                    os_mkdir_mock.assert_called_with(self._repo_path, mode=0o775)
                with self.assertRaises(Exception):  # noqa: B017
                    # uid mismatch
                    with unittest.mock.patch("os.getuid", return_value=1001):
                        redfish_bios_firmware_dumps.RedfishBIOSFirmwareDumps()

        from redfish_common_routes import Redfish

        redfish = Redfish()
        redfish.bios_firmware_dumps = bios_firmware_dumps
        redfish.setup_redfish_common_routes(webapp)
        return webapp
