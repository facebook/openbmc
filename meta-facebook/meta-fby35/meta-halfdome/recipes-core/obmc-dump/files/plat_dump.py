#!/usr/bin/env python3

# Half Dome (AMD Bergamo): extra registers collected in obmc-dump.
#
# On top of the common obmc-dump output, this adds the CPLD/GPIO registers
# that latch power-sequencing faults, for the sled baseboard and both
# populated slots (1 and 3):
#   - Sled baseboard CPLD              (i2c bus 12)
#   - Server MB CPLD                   (i2c bus 3+slot)
#   - Server MB CPLD BIC IO expander   (IPMB via bic-util)
#   - Bridge-IC GPIO                   (bic-util slotN --get_gpio)
#
# Each read is defined by its own function returning [command, file_name];
# obmc-dump runs the command and writes the output to <file_name>.txt.

_SLOTS = (1, 3)

_BIC_CPLD_OFFSETS = (
    "0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 "
    "0x09 0x0a 0x0b 0x0c 0x0d 0x0e 0x0f 0x10"
)


def bb_cpld_dump():
    """Sled baseboard CPLD (bus 12, 0x0f, offsets 0x00-0x35)."""
    return [["i2cdump", "-f", "-y", "-r", "0x00-0x35", "12", "0x0f"], "cpld-bb-dump"]


def mb_cpld_dump(slot):
    """Server MB CPLD (bus 3+slot, 0x0f). Offset 0x0A latches the failed rail."""
    return [
        ["i2cdump", "-f", "-y", "-r", "0x00-0x29", str(3 + slot), "0x0f"],
        "cpld-mb-dump-slot%d" % slot,
    ]


def bic_cpld_dump(slot):
    """Server MB CPLD BIC-side IO expander, read over IPMB via bic-util."""
    return [
        [
            "sh",
            "-c",
            "for off in %s; do "
            "bic-util slot%d 0x18 0x52 0x00 0x42 0x01 $off 2>/dev/null | tr -d '\\n'; "
            "echo -n ' '; "
            "done; echo" % (_BIC_CPLD_OFFSETS, slot),
        ],
        "bic-cpld-dump-slot%d" % slot,
    ]


def bic_gpio_dump(slot):
    """Bridge-IC GPIO state."""
    return [["bic-util", "slot%d" % slot, "--get_gpio"], "bic-gpio-dump-slot%d" % slot]


PLAT_PATHS = []

PLAT_COMMANDS = [bb_cpld_dump()]
for _slot in _SLOTS:
    PLAT_COMMANDS += [
        mb_cpld_dump(_slot),
        bic_cpld_dump(_slot),
        bic_gpio_dump(_slot),
    ]
