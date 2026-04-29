inherit zephyr-sample


ZEPHYR_MAKE_OUTPUT ?= " \
    zephyr_openamp_rsc_table.elf \
    zephyr_openamp_rsc_table.bin \
    zephyr_openamp_rsc_table.efi \
    "

ZEPHYR_SRC_DIR = "${ZEPHYR_BASE}/samples/subsys/ipc/openamp_rsc_table"

COMPATIBLE_MACHINE = "(stm32mp157c-dk2)"
