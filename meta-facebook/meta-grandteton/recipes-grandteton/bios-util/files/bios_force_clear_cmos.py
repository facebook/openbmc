from libgpio import GPIO, GPIODirection, GPIOValue
import time

def force_clear_cmos(fru):
    if fru != 1:
        raise ValueError(f"Unsupported FRU: {fru}")
    with GPIO(shadow="FM_PFR_OVR_RTC_R") as pin:
        try:
            pin.set_value(GPIOValue.HIGH)
            time.sleep(1)
            pin.set_value(GPIOValue.LOW)
        except Exception:
            pin.set_value(GPIOValue.LOW)

