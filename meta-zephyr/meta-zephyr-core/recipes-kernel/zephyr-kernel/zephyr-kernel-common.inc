# Common settings for all Zephyr recipes

ZEPHYR_INHERIT_CLASSES += "zephyr cmake"
inherit ${ZEPHYR_INHERIT_CLASSES}

# There shouldn't be a manifest for zephyr kernels since there is no root
# filesystem.
IMAGE_NO_MANIFEST = "1"

ZEPHYR_TOOLCHAIN_VARIANT ?= "cross-compile"
require recipes-kernel/zephyr-kernel/zephyr-toolchain-${ZEPHYR_TOOLCHAIN_VARIANT}.inc

ZEPHYR_MAKE_OUTPUT ?= "\
    zephyr.elf \
    zephyr.bin \
    zephyr.efi \
    "

EXTRA_OECMAKE = "\
    -DZEPHYR_BASE=${ZEPHYR_BASE} \
    -DBOARD=${BOARD} \
    -DBOARD_DIR=${BOARD_DIR} \
    -DARCH_DIR=${ARCH_DIR} \
    -DBOARD_ROOT=${BOARD_ROOT} \
    -DBOARD_EXTENSION_DIRS=${BOARD_EXTENSION_DIRS} \
    -DSOC_ROOT=${SOC_ROOT} \
    -DZEPHYR_TOOLCHAIN_VARIANT=${ZEPHYR_TOOLCHAIN_VARIANT} \
    -DZEPHYR_MODULES=${ZEPHYR_MODULES} \
    "

ZEPHYR_EXTRA_MODULES = ""

EXTRA_OECMAKE:append = "  -DZEPHYR_EXTRA_MODULES=${ZEPHYR_EXTRA_MODULES}"

export ZEPHYR_BASE = "${S}/zephyr"

DEPENDS += "gperf-native"

DEPENDS:append:qemuall = " qemu-native qemu-helper-native"

# The makefiles are explicit about the flags they want, so don't unset
# them so zephyr flags actually get used.
# This is done here rather than in the task so that things still work
# in devshell.

python () {
    d.delVar('CFLAGS')
    d.delVar('CXXFLAGS')
    d.delVar('LDFLAGS')
}

OE_TERMINAL_EXPORTS += "BOARD"
OE_TERMINAL_EXPORTS += "ZEPHYR_SRC_DIR"
OE_TERMINAL_EXPORTS += "ZEPHYR_BASE"

IMAGE_FSTYPES = "elf bin"

do_configure:prepend() {
    # Zephyr expects CPPFLAGS as cmake argument as and ignores env variables.
    unset CPPFLAGS
}
