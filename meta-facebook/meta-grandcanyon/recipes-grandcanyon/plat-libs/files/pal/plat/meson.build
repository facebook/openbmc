pal_headers += [
    'pal_power.h',
    ]

pal_deps += [
    cc.find_library('fbgc_common'),
    cc.find_library('fbgc_fruid'),
    cc.find_library('obmc-sensors'),
    cc.find_library('obmc-i2c'),
    cc.find_library('gpio-ctrl'),
    cc.find_library('fbgc_gpio'),
    cc.find_library('exp'),
    cc.find_library('bic'),
    cc.find_library('ncsi'),
    cc.find_library('nl-wrapper'),
    dependency('libphymem'),
    cc.find_library('nvme-mi'),
    ]

pal_sources += [
    'pal_sensors.c',
    'pal_power.c',
    ]
