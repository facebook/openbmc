pal_headers += [
    'pal_health.h',
    'pal_power.h',
    'pal_cfg.h',
    'pal_common.h',
    'pal_sensors.h',
    'pal_mb_sensors.h',
    'pal_swb_sensors.h',
    'pal_hgx_sensors.h',
    'pal_bb_sensors.h',
    'pal_def.h',
    'pal_def_cover.h',
    'pal_meb_sensors.h',
    'pal_acb_sensors.h'
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
    'pal_hgx_sensors.c',
    'pal_cover.c',
    'pal_meb_sensors.c',
    'pal_acb_sensors.c'
    ]

pal_deps += [
    cc.find_library('apml64'),
    dependency('libgpio-ctrl'),
    dependency('libhal'),
    dependency('libnodemanager'),
    dependency('libobmc-i2c'),
    cc.find_library('peci'),
    dependency('libpeci_sensors'),
    cc.find_library('sensor-correction'),
    dependency('libdimm'),
    dependency('libpldm-oem'),
    dependency('libhgx'),
    dependency('libmisc-utils'),
    dependency('libncsi'),
    dependency('libnl-wrapper'),
    dependency('libobmc-pmbus'),
    dependency('libobmc-sensors'),
    dependency('libpldm-oem'),
    dependency('libretimer'),
    ]
