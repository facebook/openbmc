pal_headers += [
    'pal_health.h',
    'pal_power.h',
    'pal_cfg.h',
    'pal_common.h',
    'pal_def.h',
    ]

pal_sources += [
    'pal_health.c',
    'pal_power.c',
    'pal_cfg.c',
    'pal_common.c',
    ]

pal_deps += [
    cc.find_library('gpio-ctrl'),
    cc.find_library('ncsi'),
    cc.find_library('nl-wrapper'),
    cc.find_library('nm'),
    cc.find_library('obmc-i2c'),
    cc.find_library('obmc-pmbus'),
    cc.find_library('obmc-sensors'),
    cc.find_library('peci'),
    ]
