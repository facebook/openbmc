
FILESEXTRAPATHS:prepend := "${THISDIR}/files/${SOC_FAMILY}:"

SRC_URI = "git://github.com/Nuvoton-Israel/linux;protocol=https;branch=${KBRANCH}"
SRC_URI += "file://defconfig \
           "
KBRANCH = "NPCM-5.10-OpenBMC"
LINUX_VERSION = "5.10.67"
SRCREV = "4e46cbb92f4cd1953e593c3044b6bf787e41e764"

LINUX_VERSION_EXTENSION = "-nuvoton"

