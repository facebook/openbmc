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
    cc.find_library('bic'),
    cc.find_library('fby3_common'),
    cc.find_library('fby3_fruid'),
    dependency('libgpio-ctrl'),
    dependency('libobmc-i2c'),
    cc.find_library('sensor-correction'),
    dependency('libncsi'),
    dependency('libnl-wrapper'),
    dependency('libobmc-sensors'),
    ]
