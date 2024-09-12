import re
from pathlib import Path

from fixmybmc.bmccheck import bmcCheck
from fixmybmc.status import Problem


@bmcCheck
def provisioning_stable_image():
    """
    Check that we are running a stable image
    """
    pattern = r"OpenBMC Release \w+-v\d+\.\d+\.\d+"
    image = Path("/etc/issue").read_text()
    match = re.match(pattern, image)

    if not match:
        return Problem(
            description=f"""This device is running a non-stable OpenBMC image:
                {image}""",
            manual_remediation="""Please retrieve the latest stable image with `fbpkg
                fetch openbmc.image.<platform>:provisioning_stable` and flash it
                using the steps described here:
                https://www.internalfb.com/intern/wiki/OpenBMC/RecoverBMC/
                """,
        )
