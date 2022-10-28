pal_headers += [
    'pal.h',
    'pal_debugcard.h',
    'pal_power.h',
    'pal-pim.h',
    'pal_sensors.h',
    ]

pal_sources += [
    'pal.c',
    'pal_debugcard.c',
    'pal_power.c',
    'pal-pim.c',
    'pal_sensors.c',
    ]

pal_deps += [
    cc.find_library('bic'),
    cc.find_library('gpio-ctrl'),
    cc.find_library('log'),
    cc.find_library('m'),
    cc.find_library('obmc-i2c'),
    cc.find_library('sensor-correction'),
    cc.find_library('wedge_eeprom'),
    dependency('libmisc-utils'),
    ]
