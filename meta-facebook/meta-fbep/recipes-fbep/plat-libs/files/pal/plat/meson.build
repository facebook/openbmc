pal_headers += [
    'pal_calibration.h',
    'pal_health.h',
    'pal_switch.h',
    ]

pal_sources += [
    'pal_health.c',
    'pal_sensors.c',
    'pal_switch.c',
    ]

pal_deps += [
    dependency('libgpio-ctrl'),
    dependency('libobmc-i2c'),
    dependency('libobmc-sensors'),
    cc.find_library('switchtec'),
    cc.find_library('asic'),
    dependency('libpex'),
    dependency('libcrypto'),
    dependency('libssl'),
    ]
