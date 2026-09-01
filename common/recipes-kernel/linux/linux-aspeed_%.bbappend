KBRANCH:bletchley = "dev-6.6"
KBRANCH:bletchley15 = "dev-6.6"
KBRANCH:clemente = "dev-6.6"
KBRANCH:yosemite4 = "dev-6.6"

LINUX_VERSION:bletchley = "6.6.105"
LINUX_VERSION:bletchley15 = "6.6.105"
LINUX_VERSION:clemente = "6.6.105"
LINUX_VERSION:yosemite4 = "6.6.105"

SRCREV:bletchley = "82a00f69c382193027bb1cf31acaf964b76a0eaa"
SRCREV:bletchley15 = "82a00f69c382193027bb1cf31acaf964b76a0eaa"
SRCREV:clemente = "82a00f69c382193027bb1cf31acaf964b76a0eaa"
SRCREV:yosemite4 = "82a00f69c382193027bb1cf31acaf964b76a0eaa"

LINUX_ASPEED_PATCHES_INC = "${@d.getVar('KBRANCH', True).replace('dev', 'linux-patches') + '.inc' if d.getVar('KBRANCH', True) else ''}"
include ${LINUX_ASPEED_PATCHES_INC}
