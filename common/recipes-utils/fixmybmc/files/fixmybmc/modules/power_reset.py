import os
import re

from datetime import datetime, timedelta

from pathlib import Path
from typing import List

from fixmybmc.bmccheck import bmcCheck
from fixmybmc.status import Problem, Warning

mnt_data_logfile = Path("/mnt/data/logfile")
mnt_data_date_format = "%Y %b %d %H:%M:%S"

fscd_logfile = Path("/var/log/fscd.log")
fscd_date_format = "%Y-%m-%d %H:%M:%S"


def get_recent_logs(file_path: Path, time_format: str, days_ago: int = 7) -> List[str]:
    cutoff_date = datetime.now() - timedelta(days=days_ago)
    time_str_len = len(datetime.now().strftime(time_format))

    recent_logs = []
    if not file_path.exists():
        return recent_logs

    with open(file_path, "rb") as f:
        f.seek(0, os.SEEK_END)
        pos = f.tell()

        while pos > 0:
            pos -= 1
            f.seek(pos)
            char = f.read(1)

            if char == b"\n" or pos == 0:
                if pos > 0:
                    f.seek(pos + 1)
                line = f.readline().decode().strip()

                try:
                    line_date = datetime.strptime(line[:time_str_len], time_format)
                    if line_date >= cutoff_date:
                        recent_logs.append(line)
                except ValueError:
                    continue
    return recent_logs


@bmcCheck
def recent_wedge_power_reset():
    """
    Check if a wedge_power reset has been performed recently (in last 7 days)
    """
    recent_logs = get_recent_logs(mnt_data_logfile, mnt_data_date_format, days_ago=7)
    pattern = "user.crit.*Power"
    reason = "Power reset performed by wedge_power.sh"

    reset_logs = [log for log in recent_logs if re.search(pattern, log)]

    if len(reset_logs) == 0:
        return None
    desc_str = f"\n{reason}\n" + "\n".join(" " + log for log in reset_logs)
    return Warning(description=(desc_str))


@bmcCheck
def recent_pemd_shutdown():
    """
    Check if a power reset has been performed by pemd recently (in last 7 days)
    """
    recent_logs = get_recent_logs(mnt_data_logfile, mnt_data_date_format, days_ago=7)
    pattern = "PEM.*shutdown"
    reason = "Power reset via pemd service (over-voltage/current/power event)"

    reset_logs = [log for log in recent_logs if re.search(pattern, log)]

    if len(reset_logs) == 0:
        return None
    desc_str = f"\n{reason}\n" + "\n".join(" " + log for log in reset_logs)
    return Problem(description=(desc_str))


@bmcCheck
def recent_fscd_shutdown():
    """
    Check if a power reset has been performed by fscd recently (in last 7 days)
    """
    recent_logs = get_recent_logs(fscd_logfile, fscd_date_format, days_ago=7)
    pattern = "host_shutdown()"
    reason = "Power reset via fscd (prevents potential system overheat)"

    reset_logs = [log for log in recent_logs if re.search(pattern, log)]

    if len(reset_logs) == 0:
        return None
    desc_str = f"\n{reason}\n" + "\n".join(" " + log for log in reset_logs)
    return Problem(description=(desc_str))
