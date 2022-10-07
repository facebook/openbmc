pal_headers += [
    'pal_health.h',
    'pal_power.h',
    'pal_cfg.h',
    'pal_common.h',
    'pal_def.h',
    'pal_sensors.h',
    'pal_mb_sensors.h',
    'pal_swb_sensors.h',
    'pal_hmc_sensors.h',
    'pal_bb_sensors.h',
    'pal_gpio.h'
    ]

pal_sources += [
    'pal_health.c',
    'pal_power.c',
    'pal_cfg.c',
    'pal_common.c',
    'pal_sensors.c',
    'pal_mb_sensors.c',
    'pal_swb_sensors.c',
    'pal_bb_sensors.c',
    'pal_hmc_sensors.c',
    'pal_cover.c'
    ]

pal_deps += [
    cc.find_library('hal_fruid'),
    cc.find_library('gpio-ctrl'),
    cc.find_library('ncsi'),
    cc.find_library('nl-wrapper'),
    cc.find_library('nm'),
    cc.find_library('obmc-i2c'),
    cc.find_library('obmc-pmbus'),
    cc.find_library('obmc-sensors'),
    cc.find_library('peci'),
    cc.find_library('peci_sensors'),
    cc.find_library('sensor-correction'),
    cc.find_library('apml64'),
    dependency('libpldm-oem'),
    dependency('libhgx'),
    ]
