BPN = "qemu-master"

DEPENDS += "glib-2.0-native zlib-native"

# Include target list functions BEFORE qemu-master-native_git.inc (which has inherit native)
require qemu-master-targets.inc

require qemu-master-native_git.inc

EXTRA_OECONF:append = " --target-list=${@get_qemu_usermode_target_list(d)} --disable-tools --disable-install-blobs --disable-guest-agent"

PACKAGECONFIG ??= "pie"
