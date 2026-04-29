inherit zephyr-sample

ZEPHYR_SRC_DIR = "${ZEPHYR_BASE}/samples/subsys/display/lvgl"

# TODO Once more machines and displays are supported, add a PACKAGECONFIG.
EXTRA_OECMAKE:append = " -DSHIELD=adafruit_2_8_tft_touch_v2"

COMPATIBLE_MACHINE = "(nrf52840dk-nrf52840)"
