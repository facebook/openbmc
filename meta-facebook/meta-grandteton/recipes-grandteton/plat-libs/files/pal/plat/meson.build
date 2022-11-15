pal_headers += [
    'pal_health.h',
    'pal_power.h',
    'pal_cfg.h',
    'pal_common.h',
    'pal_sensors.h',
    'pal_mb_sensors.h',
    'pal_swb_sensors.h',
    'pal_hmc_sensors.h',
    'pal_bb_sensors.h',
    'pal_def.h',
    'pal_def_cover.h'
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
    cc.find_library('apml64'),
    cc.find_library('gpio-ctrl'),
    cc.find_library('hal_fruid'),
    cc.find_library('nm'),
    cc.find_library('obmc-i2c'),
    cc.find_library('peci'),
    cc.find_library('peci_sensors'),
    cc.find_library('sensor-correction'),
    dependency('libdimm'),
    dependency('libpldm-oem'),
    dependency('libhgx'),
    dependency('libncsi'),
    dependency('libnl-wrapper'),
    dependency('libobmc-pmbus'),
    dependency('libobmc-sensors'),
    dependency('libpldm-oem'),
    ]
