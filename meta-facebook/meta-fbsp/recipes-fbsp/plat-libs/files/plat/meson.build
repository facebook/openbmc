install_headers(
    'pal.h',
    'pal_power.h',
    'pal_sensors.h',
    'pal_health.h',
    subdir: 'openbmc')

pal_sources += [
    'plat/pal.c',
    'plat/pal_power.c',
    'plat/pal_sensors.c',
    'plat/pal_health.c'
    ]

pal_deps += [
    cc.find_library('obmc-i2c'),
    cc.find_library('gpio-ctrl'),
    cc.find_library('obmc-sensors'),
    cc.find_library('nm'),
    cc.find_library('peci'),
    cc.find_library('me'),
    cc.find_library('nl-wrapper'),
    ]
