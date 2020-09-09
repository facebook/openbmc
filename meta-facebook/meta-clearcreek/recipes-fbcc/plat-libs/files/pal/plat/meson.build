pal_headers += [
    'pal_power.h',
    'pal_sensors.h',
    'pal_switch.h',
    ]

pal_sources += [
    'pal_power.c',
    'pal_sensors.c',
    'pal_switch.c',
    ]

pal_deps += [
    cc.find_library('gpio-ctrl'),
    cc.find_library('obmc-i2c'),
    cc.find_library('obmc-sensors'),
    cc.find_library('switchtec'),
    dependency('libcrypto'),
    dependency('libssl'),
    ]