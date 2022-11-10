srcs += [
    'ast_bic.cpp',
    'bic_bios.cpp',
    'bic_capsule.cpp',
    'bic_cpld_ext.cpp',
    'bic_expansion_vr.cpp',
    'bic_fw_ext.cpp',
    'bic_m2_dev.cpp',
    'bic_pcie_sw.cpp',
    'bic_vr.cpp',
    'bmc_cpld.cpp',
    'bmc_cpld_capsule.cpp',
    'expansion.cpp',
    'mcu_fw.cpp',
    'nic_ext.cpp',
    'platform.cpp',
    'usbdbg.cpp',
]

deps += [
    cc.find_library('bic'),
    cc.find_library('fby3_common'),
    dependency('libfpga'),
    dependency('libmcu'),
    dependency('libnl-wrapper'),
]
