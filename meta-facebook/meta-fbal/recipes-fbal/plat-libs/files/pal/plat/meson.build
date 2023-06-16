pal_headers += [
    'pal_cm.h',
    'pal_ep.h',
    'pal_cc.h',
    'pal_health.h',
    'pal_power.h',
    'pal_sbmc.h',
    ]

pal_sources += [
    'pal_cm.c',
    'pal_ep.c',
    'pal_cc.c',
    'pal_health.c',
    'pal_power.c',
    'pal_sbmc.c',
    'pal_sensors.c',
    ]

pal_deps += [
    cc.find_library('fbal-fruid'),
    dependency('libgpio-ctrl'),
    dependency('libnodemanager'),
    dependency('libobmc-i2c'),
    dependency('libobmc-pmbus'),
    cc.find_library('peci'),
    dependency('libncsi'),
    dependency('libnl-wrapper'),
    dependency('libobmc-sensors'),
    ]
