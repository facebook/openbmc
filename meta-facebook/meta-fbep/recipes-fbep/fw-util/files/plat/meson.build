srcs += [
    'asic.cpp',
    'cpld.cpp',
    'mcu_fw.cpp',
    'nic.cpp',
    'platform.cpp',
    'switch.cpp',
    'usbdbg.cpp',
    'vr_fw.cpp',
]

deps += [
    cc.find_library('asic'),
    dependency('libast-jtag'),
    dependency('libfpga'),
    dependency('libmcu'),
    dependency('libvr'),
]
