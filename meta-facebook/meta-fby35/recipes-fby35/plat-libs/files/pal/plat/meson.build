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
    cc.find_library('sensor-correction'),
    dependency('libbic'),
    dependency('libfby35-fruid'),
    dependency('libgpio-ctrl'),
    dependency('libmisc-utils'),
    dependency('libncsi'),
    dependency('libnl-wrapper'),
    dependency('libobmc-i2c'),
    dependency('libobmc-sensors'),
    ]
