srcs += [
    'bios.cpp',
    'cpld.cpp',
    'me.cpp',
    'platform.cpp',
    'usbdbg.cpp',
    'vr.cpp',
]

deps += [
    cc.find_library('bios'),
    cc.find_library('cpld'),
    cc.find_library('me'),
    cc.find_library('vr'),
    dependency('libgpio'),
    dependency('libocpdbg-lcd'),
]
