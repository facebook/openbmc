import asyncio
import io
import signal
import sys
import threading
import traceback

from aiohttp.log import server_logger


def install_sigusr_stack_logger():
    # Log thread/task stacks to /tmp/rest.log on SIGUSR1
    signal.signal(signal.SIGUSR1, lambda *args: _log_stacks())


def _log_stacks():
    # Log task stacks
    try:
        f = io.StringIO()
        loop = asyncio.get_event_loop()
        for task in asyncio.all_tasks(loop):
            f.write("=== Stack for task: " + repr(task) + " ===")
            task.print_stack(file=f)
            f.write("\n")
        server_logger.info(f.getvalue())

    except Exception:
        server_logger.exception(
            "Suppressed exception while inspecting event loop tasks"
        )

    # Log thread stacks
    try:
        for th in threading.enumerate():
            try:
                stack_str = (
                    "".join(traceback.format_stack(sys._current_frames()[th.ident]))
                    + "\n"
                )
                server_logger.info(
                    "=== Stack for thread: " + repr(th) + " ===\n" + stack_str
                )

            except KeyError:
                pass
    except Exception:
        server_logger.exception("Suppressed exception while inspecting thread stacks")
