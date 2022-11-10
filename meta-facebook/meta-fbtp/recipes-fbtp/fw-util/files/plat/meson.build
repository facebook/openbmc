srcs += [
    'bios.cpp',
    'cpld.cpp',
    'mcu_fw.cpp',
    'me.cpp',
    'platform.cpp',
    'usbdbg.cpp',
    'vr.cpp',
]

deps += [
    cc.find_library('me'),
    cc.find_library('vr'),
    dependency('libfpga'),
    dependency('libgpio'),
    dependency('libmcu'),
]
