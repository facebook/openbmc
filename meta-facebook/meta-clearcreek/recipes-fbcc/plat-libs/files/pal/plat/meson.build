pal_headers += [
    'pal_power.h',
    'pal_sensors.h',
    'pal_switch.h',
    'pal_health.h',
    ]

pal_sources += [
    'pal_power.c',
    'pal_sensors.c',
    'pal_switch.c',
    'pal_health.c',
    ]

pal_deps += [
    dependency('libgpio-ctrl'),
    dependency('libpex'),
    dependency('libobmc-i2c'),
    cc.find_library('switchtec'),
    dependency('libcrypto'),
    dependency('libobmc-sensors'),
    dependency('libssl'),
    dependency('libmisc-utils')
    ]
