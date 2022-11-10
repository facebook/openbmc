srcs += [
    'bios.cpp',
    'cm.cpp',
    'cpld.cpp',
    'mcu_fw.cpp',
    'me.cpp',
    'nic_ext.cpp',
    'platform.cpp',
    'usbdbg.cpp',
    'vr_fw.cpp',
]

deps += [
    cc.find_library('libnm'),
    dependency('libast-jtag'),
    dependency('libfpga'),
    dependency('libmcu'),
    dependency('libnl-wrapper'),
    dependency('libvr'),
]
