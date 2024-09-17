from typing import Optional

import fixmybmc.platform_utils as platform_utils

from fixmybmc.bmccheck import bmcCheck
from fixmybmc.status import Problem
from fixmybmc.utils import check_cmd, run_cmd


@bmcCheck
def sshd_running():
    """
    Check if sshd is running
    """

    check_cmd = ""
    manual_remediation = ""
    if platform_utils.running_systemd():
        unit_name = get_ssh_unit_name()
        if not unit_name:
            return Problem(
                description="Unable to determine ssh unit name",
            )
        check_cmd = f"systemctl status {unit_name}"
        manual_remediation = "Try to restart sshd: systemctl restart sshd"
    else:  # sysV
        check_cmd = "/etc/init.d/sshd status"
        manual_remediation = "Try to restart sshd: /etc/init.d/sshd restart"

    status = run_cmd(check_cmd.split(" "), check=False)

    if status.returncode != 0:
        return Problem(
            description="sshd is not running properly.",
            manual_remediation=manual_remediation,
        )


def get_ssh_unit_name() -> Optional[str]:
    """
    Some systemd systems use sshd.socket for per-connection daemons, while
    others use sshd.service.
    """
    for unit_name in ["sshd.socket", "sshd.service"]:
        if check_cmd(["systemctl", "is-enabled", unit_name]):
            return unit_name
    return None
