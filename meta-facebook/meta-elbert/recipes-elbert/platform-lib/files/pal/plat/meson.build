pal_headers += [
    'pal.h',
    'pal_sensors.h',
    ]

pal_sources += [
    'pal.c',
    'pal_sensors.c',
    ]

pal_deps += [
    cc.find_library('elbert_eeprom'),
    cc.find_library('gpio-ctrl'),
    cc.find_library('log'),
    cc.find_library('m'),
    cc.find_library('obmc-i2c'),
    cc.find_library('sensor-correction'),
    cc.find_library('wedge_eeprom'),
    dependency('libmisc-utils'),
    ]
