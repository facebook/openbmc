pal_headers += [
    'pal_power.h',
    'pal_health.h',
    ]

pal_sources += [
    'pal_power.c',
    'pal_sensors.c',
    'pal_health.c',
    ]

pal_deps += [
    cc.find_library('obmc-i2c'),
    cc.find_library('gpio-ctrl'),
    dependency('libobmc-sensors'),
    cc.find_library('nm'),
    cc.find_library('peci'),
    cc.find_library('me'),
    cc.find_library('nl-wrapper'),
    ]
