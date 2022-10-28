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
    cc.find_library('gpio-ctrl'),
    cc.find_library('obmc-i2c'),
    dependency('libobmc-sensors'),
    cc.find_library('switchtec'),
    cc.find_library('asic'),
    cc.find_library('libpex'),
    dependency('libcrypto'),
    dependency('libssl'),
    ]
