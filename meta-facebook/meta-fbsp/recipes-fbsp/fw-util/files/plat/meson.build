srcs += [
    'bios.cpp',
    'cpld.cpp',
    'fscd.cpp',
    'mcu_fw.cpp',
    'me.cpp',
    'nic_ext.cpp',
    'platform.cpp',
    'usbdbg.cpp',
    'vr_fw.cpp',
]

deps += [
    dependency('libast-jtag'),
    dependency('libfpga'),
    dependency('libmcu'),
    dependency('libnl-wrapper'),
    cc.find_library('nm'),
    dependency('libvr'),
]
