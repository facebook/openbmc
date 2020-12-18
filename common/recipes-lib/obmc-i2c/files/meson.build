project('libobmc-i2c', 'c',
    version: '0.1',
    license: 'GPL2',
    # Meson 0.40 only supports c++1z as an alias for c++17.
    default_options: [
        'werror=true',
        'cpp_std=c++1z',
    ],
    meson_version: '>=0.40')

install_headers(
    'i2c_cdev.h',
    'i2c_core.h',
    'i2c_device.h',
    'i2c_mslave.h',
    'i2c_sysfs.h',
    'smbus.h',
    'obmc-i2c.h',
    subdir: 'openbmc')

cc = meson.get_compiler('c')
libs = [
    cc.find_library('misc-utils'),
    dependency('liblog'),
]

srcs = files(
    'i2c_cdev.c',
    'i2c_device.c',
    'i2c_mslave.c',
    'i2c_sysfs.c',
)

# OpenBMC I2C Library
obmc_i2c_lib = shared_library('obmc-i2c', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

pkg = import('pkgconfig')
pkg.generate(libraries: [obmc_i2c_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'OpenBMC I2C Library')
