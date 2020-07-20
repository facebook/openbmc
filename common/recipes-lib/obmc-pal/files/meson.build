project('obmc-pal', 'c',
    version: '0.2',
    license: 'LGPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

cc = meson.get_compiler('c')

# Pass -Dmachine=<machine> to meson to set __MACHINE__ preprocessor macro.
add_global_arguments('-D__MACHINE__=' + get_option('machine'),
    language: 'c')

# Define header files.
pal_headers = [
    'pal.h',
    'pal_sensors.h',
    ]

# Add additional source files.
pal_sources = [
    'obmc-pal.c',
    'obmc_pal_sensors.c',
    'pal.c',
    ]

# Add additional library dependencies.
pal_deps = [
    cc.find_library('ipmi'),
    cc.find_library('rt'),
    dependency('threads'),
    dependency('libipmb'),
    dependency('libkv'),
    ]

# Import the plat-specific subdirectory for platform
# headers/sources/dependencies.
subdir('plat')

# Instead headers.
install_headers(
    pal_headers,
    subdir: 'openbmc')

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
