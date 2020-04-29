pal_headers += [  
    'pal.h',
    'pal_sensors.h',
    ]

pal_sources += [
    'machine_config.c',
    'pal.c',
    ]

pal_deps += [
    cc.find_library('gpio-ctrl'),
    cc.find_library('me'),
    cc.find_library('misc-utils'),
    cc.find_library('obmc-i2c'),
    cc.find_library('obmc-sensors'),
    cc.find_library('sensor-correction'),
    cc.find_library('vr'),
    ]
