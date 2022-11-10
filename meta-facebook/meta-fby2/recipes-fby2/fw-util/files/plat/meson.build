srcs += [
    'bic_bios_ext.cpp',
    'bic_cpld_ext.cpp',
    'bic_me_ext.cpp',
    'm2_dev.cpp',
    'mcu_fw.cpp',
    'nic_ext.cpp',
    'pcie_switch.cpp',
    'platform.cpp',
    'usbdbg.cpp',
    'vr.cpp',
]

deps += [
    cc.find_library('bic'),
    cc.find_library('fby2_common'),
    cc.find_library('fby2_sensor'),
    dependency('libipmb'),
    dependency('libipmi'),
    dependency('libmcu'),
    dependency('libnl-wrapper'),
]
