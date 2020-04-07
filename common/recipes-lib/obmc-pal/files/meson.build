project('obmc-pal', 'c',
    version: '0.2',
    license: 'LGPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

cc = meson.get_compiler('c')

# Pass -Dmachine=<machine> to meson to set __MACHINE__ preprocessor macro.
add_global_arguments('-D__MACHINE__=' + get_option('machine'),
    language: 'c')

# Install header files.
install_headers(
    'obmc-pal.h',
    'obmc_pal_sensors.h',
    subdir: 'openbmc')

# Add additional source files.
pal_sources = [
    'obmc-pal.c',
    'obmc_pal_sensors.c'
    ]

# Add additional library dependencies.
pal_deps = [
    cc.find_library('ipmi'),
    cc.find_library('rt'),
    dependency('libipmb'),
    dependency('libkv'),
    ]

# Import the plat-specific subdirectory for platform
# headers/sources/dependencies.
subdir('plat')

# Build shared library.
pal_lib = shared_library('pal',
    pal_sources,
    dependencies: pal_deps,
    version: meson.project_version(),
    install: true)

# Create pkgconfig.
pkg = import('pkgconfig')
pkg.generate(libraries: [pal_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'openbmc PAL library')
