SUMMARY = "Facebook OpenBMC Python packages"

LICENSE = "GPL-2.0-or-later"
PR = "r1"

inherit packagegroup

# After dunfell (gatesgarth and above), python3-typing is included in \
# python3-core and no longer exists.
def pg_python3_typing(d):
    distro = d.getVar('DISTRO_CODENAME', True)
    if distro == 'rocko' or distro == 'dunfell':
        return "python3-typing"
    return ""

RDEPENDS:${PN} += " \
  python3-asyncio \
  python3-compression \
  python3-core \
  python3-crypt \
  python3-ctypes \
  python3-datetime \
  python3-email \
  python3-fcntl \
  python3-io \
  python3-json \
  python3-mmap \
  python3-mime \
  python3-misc \
  python3-multiprocessing \
  python3-netserver \
  python3-pexpect \
  python3-pickle \
  python3-shell \
  python3-syslog \
  python3-threading \
  python3-pkgutil \ 
  python3-unittest \
  python3-debugger \
  ${@pg_python3_typing(d)} \
"
