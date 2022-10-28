pal_headers += [
    'pal.h',
    'pal-led.h',
    'pal-power.h',
    'pal-sensors.h',
    'pal-debugcard.h',
    ]

pal_sources += [
    'pal.c',
    'pal-ipmi.c',
    'pal-led.c',
    'pal-power.c',
    'pal-sensors.c',
    'pal-debugcard.c',
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
