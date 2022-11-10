srcs += [
    'nic_ext.cpp',
    'bmc_fpga.cpp',
    'platform.cpp',
    'bic_cpld.cpp',
    'bic_vr.cpp',
    'scc_exp.cpp',
    'ioc.cpp',
]

deps += [
    cc.find_library('bic'),
    cc.find_library('fbgc_common'),
    cc.find_library('libexp'),
    dependency('libfpga'),
    dependency('libipc'),
    dependency('libncsi'),
    dependency('libnl-wrapper'),
]
