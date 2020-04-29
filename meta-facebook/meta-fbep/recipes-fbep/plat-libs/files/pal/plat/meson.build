pal_headers += [
    'pal.h',
    'pal_calibration.h',
    'pal_gpu.h',
    'pal_health.h',
    'pal_sensors.h',
    'pal_switch.h',
    ]

pal_sources += [
    'pal.c',
    'pal_gpu.c',
    'pal_health.c',
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
