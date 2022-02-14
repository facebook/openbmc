import os
import re
import uuid
from contextlib import suppress
from functools import wraps
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
from redfish_computer_system import get_compute_system_names

try:
    import psutil

    has_psutil = True
except ImportError:
    has_psutil = False


_dump_id_validator = re.compile(r"[A-Za-z0-9\-._]{,100}")
_server_name_validator = re.compile(r"[A-Za-z0-9\-._]{,100}")


def _check_dump_id(dump_id):
    if not _dump_id_validator.match(dump_id):
        return "dump_id is invalid"
    return None


def _webassert_valid_dump_id(method):
    @wraps(method)
    def _impl(self, request: web.Request) -> web.Response:
        dump_id = request.match_info["DumpID"]
        dump_id_error = _check_dump_id(dump_id)
        if dump_id_error is not None:
            return RedfishError(
                status=400,
                message=dump_id_error,
            ).web_response()
        return method(self, request)

    return _impl


def _webassert_valid_server_name(method):
    @wraps(method)
    def _impl(self, request: web.Request):
        server_name = request.match_info["server_name"]
        if server_name == "":
            return RedfishError(
                status=400,
                message="server_name is empty",
            ).web_response()
        if not _server_name_validator.match(server_name):
            return RedfishError(
                status=400,
                message="server_name is not valid",
            ).web_response()
        if server_name not in self.valid_server_names:
            return RedfishError(
                status=404,
                message="server_name is not known",
            ).web_response()
        return method(self, request)

    return _impl


def _get_dump_header(server_name: str, dump_id: str) -> Dict[str, Any]:
    return {
        "@odata.id": "/redfish/v1/Systems/{}/Bios/FirmwareDumps/{}".format(
            server_name, dump_id
        ),
        "@odata.type": "#BIOSFirmwareDump.v1_0_0.BIOSFirmwareDump",
        "Id": dump_id,
        "Name": "BIOS firmware dump",
        "Actions": {
            "#BIOSFirmwareDump.ReadContent": {
                "target": (
                    "/redfish/v1/Systems/{}/Bios/FirmwareDumps/"
                    "{}/Actions/BIOSFirmwareDump.ReadContent"
                ).format(server_name, dump_id)
            },
        },
    }


class RedfishBIOSFirmwareDumps:
    def __init__(
        self,
        images_dir="/tmp/restapi-bios_images",
        image_count_limit=1,
        valid_server_names=None,
    ):
        self.images_dir = images_dir
        self.image_count_limit = image_count_limit
        self.valid_server_names = valid_server_names
        if self.valid_server_names is None:
            self.valid_server_names = get_compute_system_names()

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
    ) -> List[Optional[Dict[str, Any]]]:
        server_dirpath = os.path.join(self.images_dir, server_name)
        dumps = []
        if os.path.exists(server_dirpath):
            for dump_id in os.listdir(server_dirpath):
                dumps.append(await self._get_dump(server_name, dump_id))
        return dumps

    @_webassert_valid_server_name
    async def get_collection_descriptor(self, request: web.Request) -> web.Response:
        server_name = request.match_info["server_name"]
        dumps = await self._dump_collection_of_server(server_name)
        body = {
            "@odata.id": "/redfish/v1/Systems/{}/Bios/FirmwareDumps".format(
                server_name
            ),
            "@odata.type": "#FirmwareDumps.v1_0_0.FirmwareDumps",
            "Id": "{} BIOS dumps".format(server_name),
            "Name": "{} BIOS dumps".format(server_name),
            "Members@odata.count": len(dumps),
            "Members": dumps,
        }
        return web.json_response(body, dumps=dumps_bytestr)

    @_webassert_valid_server_name
    async def create_dump(self, request: web.Request) -> web.Response:
        server_name = request.match_info["server_name"]
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

        if "DumpID" in request.match_info and request.match_info["DumpID"] != "":
            dump_id = request.match_info["DumpID"]
        else:
            dump_id = uuid.uuid4().hex
        dump_id_error = _check_dump_id(dump_id)
        if dump_id_error is not None:
            return RedfishError(
                status=400,
                message=dump_id_error,
            ).web_response()
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

    @_webassert_valid_server_name
    @_webassert_valid_dump_id
    async def get_dump_descriptor(self, request: web.Request) -> web.Response:
        server_name = request.match_info["server_name"]
        dump_id = request.match_info["DumpID"]
        dump_info = await self._get_dump(server_name, dump_id)
        if dump_info is None:
            return web.json_response(status=404)
        return web.json_response(dump_info, dumps=dumps_bytestr)

    @_webassert_valid_server_name
    @_webassert_valid_dump_id
    async def read_dump_content(self, request: web.Request) -> web.Response:
        server_name = request.match_info["server_name"]
        dump_id = request.match_info["DumpID"]
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
            dump_dirpath = os.path.join(self.images_dir, server_name, dump_id)
            with suppress(FileNotFoundError):
                with open(os.path.join(dump_dirpath, "fwutil-pid"), "r") as file:
                    try:
                        fwutil_pid = int(file.read())
                    except ValueError:
                        return None
                    try:
                        proc = psutil.Process(pid=fwutil_pid)
                    except psutil.NoSuchProcess:
                        return None
                    for child in proc.children(recursive=True):
                        server_logger.debug("child.name() is '{}'".format(child.name()))
                        if child.name() == "fw-util":
                            return child
            return None

    @_webassert_valid_server_name
    @_webassert_valid_dump_id
    async def delete_dump(self, request: web.Request) -> web.Response:
        server_name = request.match_info["server_name"]
        dump_id = request.match_info["DumpID"]
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
            if not os.path.exists(os.path.join(dump_dirpath, "exitcode")):
                return RedfishError(
                    status=400,
                    message=(
                        "unable to delete the dump,"
                        " while it is in process of dumping"
                    ),
                ).web_response()
        # delete all files
        for file_name in ["image", "exitcode", "err", "log", "fwutil-pid"]:
            with suppress(FileNotFoundError):
                os.remove(os.path.join(dump_dirpath, file_name))
        # delete the directory
        with suppress(FileNotFoundError):
            os.rmdir(dump_dirpath)
        return web.json_response(dump_info, dumps=dumps_bytestr)
