from fixmybmc.bmccheck import bmcCheck
from fixmybmc.status import Problem
from fixmybmc.utils import run_cmd


@bmcCheck
def fpga_ver_ok():
    """
    Check if output from fpga_ver.sh is ok
    """
    check_cmd = "fpga_ver.sh"

    status = run_cmd(check_cmd.split(" "))
    if status.returncode == 0:
        return None
    return Problem(
        description=(
            "fpga_ver.sh returned error. Check the below output for more details."
        ),
        cmd_status=status,
        manual_remediation=(
            "Run `wedge_power.sh reset -s` to powercycle the device then run "
            "fixmybmc again and see if it has resolved. If not, send this error "
            "to ENS Break/Fix."
        ),
    )
