require qemu-master_git.inc

inherit native

EXTRA_OEMAKE:append = " LD='${LD}' AR='${AR}' OBJCOPY='${OBJCOPY}' LDFLAGS='${LDFLAGS}'"

LDFLAGS += "-fuse-ld=bfd"
