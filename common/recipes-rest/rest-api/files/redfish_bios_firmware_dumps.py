import os
import re
import uuid
from contextlib import suppress
from typing import (
    Any,
    Dict,
    List,
    Optional,
)

from aiohttp import web
from aiohttp.log import server_logger
from common_utils import dumps_bytestr
from redfish_base import (
    RedfishError,
)

try:
    import psutil

    has_psutil = True
except ImportError:
    has_psutil = False


class InvalidDumpID(AssertionError):
    pass


class InvalidServerName(AssertionError):
    pass


_dump_id_validator = re.compile(r"[A-Za-z0-9\-._]{,100}")
_server_name_validator = re.compile(r"[A-Za-z0-9\-._]{,100}")


def _assert_valid_dump_id(dump_id: str):
    if not _dump_id_validator.match(dump_id):
        raise InvalidDumpID("dump_id is not valid")


def _assert_valid_server_name(server_name: str):
    if not _server_name_validator.match(server_name):
        raise InvalidServerName("server_name is not valid")


def _assert_valid_server_name_and_dump_id(server_name: str, dump_id: str):
    _assert_valid_server_name(server_name)
    _assert_valid_dump_id(dump_id)


def _get_dump_header(server_name: str, dump_id: str) -> Optional[Dict[str, Any]]:
    _assert_valid_server_name_and_dump_id(server_name, dump_id)
    return {
        "@odata.id": "/redfish/v1/Systems/{}/Bios/FirmwareDumps/{}".format(
            server_name, dump_id
        ),
        "Id": dump_id,
        "Name": "BIOS firmware dump",
        "Actions": {
            "#BIOSFirmwareDump.ReadContent": {
                "target": "/redfish/v1/Systems/{}/Bios/FirmwareDumps/{}/Actions/BIOSFirmwareDump.ReadContent".format(
                    server_name, dump_id
                )
            },
        },
    }


class RedfishBIOSFirmwareDumps:
    def __init__(
        self,
        images_dir="/tmp/restapi-bios_images",
        image_count_limit=1,
    ):
        self.images_dir = images_dir
        self.image_count_limit = image_count_limit

        with suppress(FileExistsError):
            os.mkdir(self.images_dir, mode=0o775)

        # Checking that nobody tries to hack us with symlinks.
        # We expect if we are the owners of the directory, then we created
        # it and there could be no malicious symlinks.
        images_dir_stat = os.stat(self.images_dir)
        if images_dir_stat.st_uid != os.getuid():
            raise Exception(
                "{} has a wrong owner, please recheck why".format(self.images_dir)
            )

    async def _dump_collection_of_server(
        self, server_name: str
    ) -> List[Dict[str, Any]]:
        _assert_valid_server_name(server_name)
        server_dirpath = os.path.join(self.images_dir, server_name)
        dumps = []
        if os.path.exists(server_dirpath):
            for dump_id in os.listdir(server_dirpath):
                dumps.append(await self._get_dump(server_name, dump_id))
        return dumps

    async def get_collection_descriptor(
        self, server_name: str, request: web.Request
    ) -> web.Response:
        _assert_valid_server_name(server_name)
        dumps = await self._dump_collection_of_server(server_name)
        body = {
            "@odata.id": "/redfish/v1/Systems/{}/Bios/FirmwareDumps".format(
                server_name
            ),
            "Name": "{} BIOS dumps".format(server_name),
            "Members@odata.count": len(dumps),
            "Members": dumps,
        }
        return web.json_response(body, dumps=dumps_bytestr)

    async def create_dump(self, server_name: str, request: web.Request) -> web.Response:
        _assert_valid_server_name(server_name)
        server_dirpath = os.path.join(self.images_dir, server_name)
        with suppress(FileExistsError):
            os.mkdir(server_dirpath)
        images_count = len(os.listdir(server_dirpath))
        if images_count >= self.image_count_limit:
            message = "already have {} images, while the limit is {}".format(
                images_count, self.image_count_limit
            )
            return RedfishError(
                status=507,
                message=message,
            ).web_response()

        if "DumpID" in request.match_info:
            dump_id = request.match_info["DumpID"]
        else:
            dump_id = uuid.uuid4().hex
        _assert_valid_dump_id(dump_id)
        output_path = os.path.join(server_dirpath, dump_id)
        try:
            os.mkdir(output_path)
        except FileExistsError:
            message = "dump ID '{}' already exists".format(dump_id)
            return RedfishError(
                status=409,
                message=message,
            ).web_response()

        env = os.environ
        env.update(
            {
                "SERVER_SLOT": server_name.replace("server", "slot"),
                "OUTPUT_PATH": output_path,
            }
        )
        process_id = os.spawnvpe(
            os.P_NOWAIT,
            "sh",
            [
                "sh",
                "-c",
                'fw-util "$SERVER_SLOT" --dump bios "$OUTPUT_PATH"/image'
                ' > "$OUTPUT_PATH"/log 2>"$OUTPUT_PATH"/err;'
                'echo $? > "$OUTPUT_PATH"/exitcode',
            ],
            env,
        )
        with open(os.path.join(output_path, "fwutil-pid"), "w") as file:
            file.write("{}".format(process_id))

        dump_info = await self._get_dump(server_name, dump_id)
        return web.json_response(dump_info, dumps=dumps_bytestr)

    async def get_dump_descriptor(
        self, server_name: str, request: web.Request
    ) -> web.Response:
        dump_id = request.match_info["DumpID"]
        _assert_valid_server_name_and_dump_id(server_name, dump_id)
        dump_info = await self._get_dump(server_name, dump_id)
        return web.json_response(dump_info, dumps=dumps_bytestr)

    async def read_dump_content(
        self, server_name: str, request: web.Request
    ) -> web.Response:
        dump_id = request.match_info["DumpID"]
        _assert_valid_server_name_and_dump_id(server_name, dump_id)
        dump_dirpath = os.path.join(self.images_dir, server_name, dump_id)
        if not os.path.exists(dump_dirpath):
            return web.Response(status=404)
        image_path = os.path.join(dump_dirpath, "image")
        try:
            file = open(image_path, "rb")
            start_pos = 0
            end_pos = -1
            if "start_pos" in request.query:
                start_pos = int(request.query["start_pos"])
            if "end_pos" in request.query:
                end_pos = int(request.query["end_pos"])
            if start_pos > 0:
                file.seek(start_pos)
            if end_pos > 0:
                body = file.read(end_pos - start_pos)
            else:
                body = file.read()
            return web.Response(
                body=body, status=200, content_type="application/octet-stream"
            )
        except FileNotFoundError:
            return web.Response(status=204)

    async def _get_dump(self, server_name, dump_id) -> Optional[Dict[str, Any]]:
        _assert_valid_server_name_and_dump_id(server_name, dump_id)
        dump_dirpath = os.path.join(self.images_dir, server_name, dump_id)
        if not os.path.exists(dump_dirpath):
            return None
        result = _get_dump_header(server_name, dump_id)
        result["Status"] = "started"
        result["Log"] = {}
        exitcode_path = os.path.join(dump_dirpath, "exitcode")
        exitcode_isset = os.path.exists(exitcode_path)
        if exitcode_isset:
            exitcode_file = open(exitcode_path, "r")
            exitcode_string = exitcode_file.read()
            if exitcode_string != "":
                exitcode = int(exitcode_string)
                result["Log"]["ExitCode"] = exitcode
                if exitcode == 0:
                    result["Status"] = "success"
                else:
                    result["Status"] = "failure"
            else:
                result["Status"] = "unknown"
        else:
            if has_psutil:
                if self._fwutil_proc(server_name, dump_id) is None:
                    result["Status"] = "interrupted"
            else:
                result["Status"] = "unknown"
        with suppress(Exception):
            debug_log_path = os.path.join(dump_dirpath, "log")
            with open(debug_log_path, "r") as file:
                result["Log"]["Debug"] = file.read()
        with suppress(Exception):
            error_log_path = os.path.join(dump_dirpath, "err")
            with open(error_log_path, "r") as file:
                result["Log"]["Error"] = file.read()

        dump_dirstat = os.stat(dump_dirpath)
        image_path = os.path.join(dump_dirpath, "image")
        if not os.path.exists(image_path):
            return result
        image_stat = os.stat(image_path)
        result["TimestampStartMS"] = int(dump_dirstat.st_atime * 1000)
        result["SizeCurrent"] = image_stat.st_size
        if exitcode_isset:
            result["TimestampEndMS"] = int(image_stat.st_ctime * 1000)
            result["SizeTotal"] = image_stat.st_size
        return result

    if has_psutil:

        def _fwutil_proc(
            self, server_name: str, dump_id: str
        ) -> Optional[psutil.Process]:
            _assert_valid_server_name_and_dump_id(server_name, dump_id)
            dump_dirpath = os.path.join(self.images_dir, server_name, dump_id)
            with suppress(FileNotFoundError):
                with open(os.path.join(dump_dirpath, "fwutil-pid"), "r") as file:
                    fwutil_pid = int(file.read())
                    with suppress(psutil.NoSuchProcess):
                        proc = psutil.Process(pid=fwutil_pid)
                        for child in proc.children(recursive=True):
                            server_logger.debug(
                                "child.name() is '{}'".format(child.name())
                            )
                            if child.name() == "fw-util":
                                return child
            return None

    async def delete_dump(self, server_name: str, request: web.Request) -> web.Response:
        dump_id = request.match_info["DumpID"]
        _assert_valid_server_name_and_dump_id(server_name, dump_id)
        dump_info = await self._get_dump(server_name, dump_id)
        if dump_info is None:
            return web.Response(status=404)
        dump_dirpath = os.path.join(self.images_dir, server_name, dump_id)
        if has_psutil:
            # kill fw-util if running
            proc = self._fwutil_proc(server_name, dump_id)
            if proc is not None:
                proc.terminate()
                try:
                    proc.wait(timeout=5)
                except psutil.TimeoutExpired:
                    return RedfishError(
                        status=504, message="timed out on killing fw-util process"
                    ).web_response()
        else:
            # ensure fw-util has already ended:
            if not os.path.exists(os.path.join(dump_dirpath, "fwutil-pid")):
                return RedfishError(
                    status=400,
                    message="unable to delete the dump, while it is in process of dumping",
                ).web_response()
        # delete all files
        for file_name in ["image", "exitcode", "err", "log", "fwutil-pid"]:
            with suppress(FileNotFoundError):
                os.remove(os.path.join(dump_dirpath, file_name))
        # delete the directory
        with suppress(FileNotFoundError):
            os.rmdir(dump_dirpath)
        return web.json_response(dump_info, dumps=dumps_bytestr)

    def get_server(self, server_name: str) -> "RedfishBIOSFirmwareServerDumps":
        return RedfishBIOSFirmwareServerDumps(self, server_name=server_name)


class RedfishBIOSFirmwareServerDumps:
    def __init__(self, handler: RedfishBIOSFirmwareDumps, server_name: str):
        self.handler = handler
        self.server_name = server_name

    async def get_collection_descriptor(self, request: web.Request):
        return await self.handler.get_collection_descriptor(self.server_name, request)

    async def get_dump_descriptor(self, request: web.Request):
        return await self.handler.get_dump_descriptor(self.server_name, request)

    async def create_dump(self, request: web.Request):
        return await self.handler.create_dump(self.server_name, request)

    async def delete_dump(self, request: web.Request):
        return await self.handler.delete_dump(self.server_name, request)

    async def read_dump_content(self, request: web.Request):
        return await self.handler.read_dump_content(self.server_name, request)
