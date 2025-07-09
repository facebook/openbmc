from fixmybmc.bmccheck import bmcCheck
from fixmybmc.remediation import remediation
from fixmybmc.status import Error, Problem
from fixmybmc.utils import run_cmd


@bmcCheck
def eth0_up():
    """
    Check if eth0 is up
    """
    check_cmd = "ip link show eth0"

    status = run_cmd(check_cmd.split(" "))
    if status.returncode != 0:
        return Error(
            description="Unable to determine eth0 state",
            cmd_status=status,
        )
    if "state UP" in status.stdout:
        return None
    return Problem(
        description="eth0 is not up.",
        remediation=restart_eth0,
    )


@remediation
def restart_eth0():
    """
    Restart eth0 interface via `ip link set eth0 down && ip link set eth0 up`
    """
    run_cmd("ip link set eth0 down".split(" "), capture_output=False)
    run_cmd("ip link set eth0 up".split(" "), capture_output=False)


@bmcCheck
def global_address_assigned():
    """
    Check if a global IPv6 address is assigned to eth0 interface
    """
    check_cmd = "ifconfig eth0"

    status = run_cmd(check_cmd.split(" "))
    if status.returncode != 0:
        return Error(
            description="Unable to get eth0 interface information",
            cmd_status=status,
        )

    # Look for IPv6 addresses with Scope:Global
    lines = status.stdout.split("\n")
    for line in lines:
        if "inet6 addr:" in line and "Scope:Global" in line:
            return None  # Found global address, check passes

    return Problem(
        description="No global IPv6 address assigned to eth0 interface",
        manual_remediation="""
        Possible issues include front-panel cable not connected, bad eeprom
        image, management network down, or bad MAC/PHY settings.""",
    )
