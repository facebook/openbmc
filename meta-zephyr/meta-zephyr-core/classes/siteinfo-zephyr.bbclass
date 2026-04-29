def iamcu_siteinfo_setfunc(archinfo, osinfo, targetinfo, d):
    archinfo['iamcu'] = "endian-little bit-32 "
    osinfo['linux'] = "common-linux common-glibc"
    targetinfo['iamcu-linux'] = "iamcu-linux"
    return archinfo, osinfo, targetinfo

SITEINFO_EXTRA_DATAFUNCS += "iamcu_siteinfo_setfunc"

def nios2_machdata_setfunc(machdata, d):
    machdata["elf"]["nios2"] = (113, 0, 0, True, 32)
    return machdata

PACKAGEQA_EXTRA_MACHDEFFUNCS += "nios2_machdata_setfunc"
