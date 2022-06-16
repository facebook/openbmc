include conf/distro/openbmc-yocto-${DISTRO_CODENAME}.inc
require conf/distro/include/fb/network.inc
require conf/distro/include/fb/packaging.inc
require conf/distro/include/fb/parallel.inc
require conf/distro/include/fb/sstate.inc
require conf/distro/include/fb/versioning.inc

DEFAULT_TIMEZONE ?= "America/Los_Angeles"

# By default disable interactive patch resolution (tasks will just fail instead):
PATCHRESOLVE = "noop"

def yocto_halt_taskname(d):
    distro = d.getVar('DISTRO_CODENAME', True)
    if distro in [ 'rocko', 'dunfell' ]:
        return "ABORT"
    return "HALT"

# Disk Space Monitoring during the build
#   This comes from the Yocto default in their local.inc.sample.
BB_DISKMON_DIRS ??= "\
    STOPTASKS,${TMPDIR},1G,100K \
    STOPTASKS,${DL_DIR},1G,100K \
    STOPTASKS,${SSTATE_DIR},1G,100K \
    STOPTASKS,/tmp,100M,100K \
    ${@yocto_halt_taskname(d)},${TMPDIR},100M,1K \
    ${@yocto_halt_taskname(d)},${DL_DIR},100M,1K \
    ${@yocto_halt_taskname(d)},${SSTATE_DIR},100M,1K \
    ${@yocto_halt_taskname(d)},/tmp,10M,1K"
