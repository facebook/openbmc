import time
import subprocess

from libgpio import GPIO, GPIODirection, GPIOValue


def _run(cmd):
    subprocess.run(cmd, check=True)


def force_clear_cmos(fru):
    if fru != 1:
        raise ValueError(f"Unsupported FRU: {fru}")

    # Read UIC location to decide A-side/B-side RTC reset pin.
    # BMC_UIC_LOCATION_IN: LOW(0) => A side, HIGH(1) => B side
    with GPIO(shadow="BMC_UIC_LOCATION_IN") as loc_pin:
        loc_pin.set_direction(GPIODirection.IN)
        loc = loc_pin.get_value()

    rtcrst = "FM_BMC_RST_B_R_RTCRST" if loc == GPIOValue.HIGH else "FM_BMC_RST_A_R_RTCRST"

    # Toggle RTCRST to clear CMOS (always return to LOW).
    with GPIO(shadow=rtcrst) as pin:
        pin.set_direction(GPIODirection.OUT)
        try:
            pin.set_value(GPIOValue.HIGH)
            time.sleep(0.2)
        finally:
            pin.set_value(GPIOValue.LOW)

    # Keep existing CPLD NIC PGM programming step.
    # i2ctransfer -y 5 w2@0x0f 0x03 0x01
    _run(["i2ctransfer", "-fy", "5", "w2@0x0f", "0x03", "0x01"])
