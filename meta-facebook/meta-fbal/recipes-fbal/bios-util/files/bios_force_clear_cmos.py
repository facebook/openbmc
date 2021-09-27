from libgpio import GPIO, GPIODirection, GPIOValue


def force_clear_cmos(fru):
    if fru != 1:
        raise ValueError(f"Unsupported FRU: {fru}")
    with GPIO(shadow="RST_RTCRST_N") as pin:
        try:
            pin.set_direction(GPIODirection.OUT)
            pin.set_value(GPIOValue.LOW)
            pin.set_direction(GPIODirection.IN)
        except Exception:
            pin.set_direction(GPIODirection.IN)
