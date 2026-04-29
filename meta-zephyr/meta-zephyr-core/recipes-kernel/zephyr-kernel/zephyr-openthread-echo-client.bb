inherit zephyr-sample

ZEPHYR_SRC_DIR = "${ZEPHYR_BASE}/samples/net/sockets/echo_client"

EXTRA_OECMAKE += "-DOVERLAY_CONFIG=overlay-ot.conf"

# The overlay config and OpenThread itself imposes some specific requirements
# towards the boards (e.g. flash layout and ieee802154 radio) so we need to
# limit to known working machines here.
COMPATIBLE_MACHINE = "(arduino-nano-33-ble)"
