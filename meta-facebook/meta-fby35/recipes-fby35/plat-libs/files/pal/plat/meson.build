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
    cc.find_library('fby35_common'),
    cc.find_library('fby35_fruid'),
    cc.find_library('misc-utils'),
    cc.find_library('ncsi'),
    cc.find_library('nl-wrapper'),
    dependency('libobmc-sensors'),
    cc.find_library('sensor-correction'),
    dependency('libbic'),
    dependency('libgpio-ctrl'),
    dependency('libobmc-i2c'),
    ]
