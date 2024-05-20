import aiohttp.web
import common_utils


async def get_board_revision(request: aiohttp.web.Request) -> aiohttp.web.Response:
    """
    Endpoint for getting board revision.
    """

    (ret, stdout, stderr) = await common_utils.async_exec(
        ["/usr/local/bin/fboss-board-revision.sh", "-r", "-s"], shell=False
    )
    if ret != 0:
        return aiohttp.web.json_response(
            {"status": "FAIL", "extra": "Command execution"}
        )

    lines = stdout.split("\n")
    if lines and lines[-1] == "":
        lines = lines[:-1]

    if len(lines) != 2:
        return aiohttp.web.json_response({"status": "FAIL", "extra": "Parse error"})

    is_respin = None if lines[0] == "unknown" else lines[0] == "1"
    human_str = lines[1]

    return aiohttp.web.json_response(
        {"status": "OK", "is_respin": is_respin, "human_readable": human_str}
    )
