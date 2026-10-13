KBRANCH:yosemite4 = "dev-6.6"

LINUX_VERSION:yosemite4 = "6.6.105"

SRCREV:yosemite4 = "82a00f69c382193027bb1cf31acaf964b76a0eaa"

LINUX_ASPEED_PATCHES_INC = "${@d.getVar('KBRANCH', True).replace('dev', 'linux-patches') + '.inc' if d.getVar('KBRANCH', True) else ''}"
include ${LINUX_ASPEED_PATCHES_INC}
