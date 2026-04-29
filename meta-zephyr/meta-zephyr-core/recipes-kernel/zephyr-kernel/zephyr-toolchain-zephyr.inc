# Additional definitions to use the Zephyr SDK

# Configure usage of the Zephyr SDK
INHIBIT_DEFAULT_DEPS = "1"
DEPENDS += "zephyr-sdk-native"
export ZEPHYR_SDK_INSTALL_DIR = "${STAGING_DIR_NATIVE}${prefix}/zephyr-sdk"
OE_TERMINAL_EXPORTS += "ZEPHYR_SDK_INSTALL_DIR"

# The Zephyr SDK does not require a CMake toolchain file
EXTRA_OECMAKE:append = " -DCMAKE_TOOLCHAIN_FILE="
deltask generate_toolchain_file

# Provide a suitable location to store the toolchain capabilites cache
ZEPHYR_USER_CACHE_DIR = "${B}/.cache"
EXTRA_OECMAKE:append = " -DUSER_CACHE_DIR=${ZEPHYR_USER_CACHE_DIR}"

# Inhibit packaging steps that require the Yocto toolchain
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"

# Set Zephyr work directory to describe toolchain accurately
MULTIMACH_TARGET_SYS = "${ZEPHYR_BOARD}-zephyr"
