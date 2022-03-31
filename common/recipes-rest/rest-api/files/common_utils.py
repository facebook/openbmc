import asyncio
import json
import re
import os
from aiohttp import web
from concurrent.futures import ThreadPoolExecutor
from typing import Dict, List, Optional, Set, Tuple, Union

from common_webapp import WebApp

common_executor = ThreadPoolExecutor(5)

# cache for endpoint_children
ENDPOINT_CHILDREN = {}  # type: Dict[str, Set[str]]


def common_force_async(func):
    # common handler will use its own executor (thread based),
    # we initentionally separated this from the executor of
    # board-specific REST handler, so that any problem in
    # common REST handlers will not interfere with board-specific
    # REST handler, and vice versa
    async def func_wrapper(*args, **kwargs):
        # Convert the possibly blocking helper function into async
        loop = asyncio.get_event_loop()
        result = await loop.run_in_executor(common_executor, func, *args, **kwargs)
        return result

    return func_wrapper


# When we call request.json() in asynchronous function, a generator
# will be returned. Upon calling next(), the generator will either :
#
# 1) return the next data as usual,
#   - OR -
# 2) throw StopIteration, with its first argument as the data
#    (this is for indicating that no more data is available)
#
# Not sure why aiohttp's request generator is implemented this way, but
# the following function will handle both of the cases mentioned above.
def get_data_from_generator(data_generator):
    data = None
    try:
        data = next(data_generator)
    except StopIteration as e:
        data = e.args[0]
    return data


def get_endpoints(path: str):
    app = WebApp.instance()
    endpoints = set()  # type: Set[str]
    splitpaths = []  # type: List[str]
    splitpaths = path.split("/")
    position = len(splitpaths)
    if path in ENDPOINT_CHILDREN:
        endpoints = ENDPOINT_CHILDREN[path]  # type: ignore
    else:
        for route in app.router.resources():
            string = str(route)
            rest_route_path = string[string.index("  ") :]
            if rest_route_path.endswith(">"):
                rest_route_path = rest_route_path[:-1]
            rest_route_path_slices = rest_route_path.split("/")
            if len(rest_route_path_slices) > position and path in string:
                endpoints.add(rest_route_path_slices[position])
        endpoints = sorted(endpoints)  # type: ignore
        ENDPOINT_CHILDREN[path] = endpoints
    return endpoints


# aiohttp allows users to pass a "dumps" function, which will convert
# different data types to JSON. This new dumps function will simply call
# the original dumps function, along with the new type handler that can
# process byte strings.
def dumps_bytestr(obj):
    # aiohttp's json_response function uses py3 JSON encoder, which
    # doesn't know how to handle a byte string. So we extend this function
    # to handle the case. This is a standard way to add a new type,
    # as stated in JSON encoder source code.
    def default_bytestr(o):
        # If the object is a byte string, it will be converted
        # to a regular string. Otherwise we move on (pass) to
        # the usual error generation routine
        try:
            o = o.decode("utf-8")
            return o
        except AttributeError:
            pass
        raise TypeError(repr(o) + " is not JSON serializable")

    # Just call default dumps function, but pass the new default function
    # that is capable of process byte strings.
    return json.dumps(obj, default=default_bytestr)


def running_systemd():
    return "systemd" in os.readlink("/proc/1/exe")


async def async_exec(
    cmd: Union[List[str], str], shell: bool = False
) -> Tuple[Optional[int], str, str]:
    if shell:
        proc = await asyncio.create_subprocess_shell(
            cmd,  # type: ignore
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
        )
    else:
        proc = await asyncio.create_subprocess_exec(
            *cmd, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE
        )

    stdout, stderr = await proc.communicate()
    data = stdout.decode()
    err = stderr.decode()

    await proc.wait()
    return proc.returncode, data, err


def parse_expand_level(request: web.Request) -> int:
    # Redfish supports the $expand query parameter
    # Spec: http://redfish.dmtf.org/schemas/DSP0266_1.7.0.html#use-of-the-expand-query-parameter-a-id-expand-parameter-a-
    params = request.rel_url.query
    expand_level = params.get("$expand", None)
    if not expand_level:
        return 0
    if expand_level == "*":
        return 999  # expands levels are handled downstream as int-s. Return 999 so we surely expand every child node
    matches = re.match(r"\.\(\$levels=(\d)\)", expand_level)
    if matches:
        return int(matches.group(1))
    try:
        return int(expand_level)
    except ValueError:
        raise web.HTTPBadRequest(
            body=json.dumps(
                {
                    "reason": "Invalid expand level supplied: {expand_level}".format(
                        expand_level=expand_level
                    )
                }
            ),
            content_type="application/json",
        )
