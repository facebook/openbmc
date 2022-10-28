pal_sources += [
    'pal_power.c',
    'pal_sensors.c',
    ]

pal_headers += [
    'pal_power.h',
    'pal_sensors.h',
    'pal.h',
    ]


pal_deps += [
    cc.find_library('fbwc_fruid'),
    cc.find_library('gpio-ctrl'),
    cc.find_library('libbic'),
    dependency('libobmc-sensors'),
    cc.find_library('ncsi'),
    cc.find_library('libexp'),
    ]
