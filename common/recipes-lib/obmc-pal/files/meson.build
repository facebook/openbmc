project('obmc-pal', 'c',
    version: '0.2',
    license: 'LGPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

cc = meson.get_compiler('c')

# Pass -Dmachine=<machine> to meson to set __MACHINE__ preprocessor macro.
add_global_arguments('-D__MACHINE__=' + get_option('machine'),
    language: 'c')

build_crashdump_amd = get_option('crashdump-amd')

# Define header files.
pal_headers = [
    'pal.h',
    'pal_sensors.h',
    ]

# Add additional source files.
pal_sources = [
    'obmc-pal.c',
    'obmc-pal-ext.c',
    'obmc_pal_sensors.c',
    'pal.c',
    ]

# Add additional library dependencies.
pal_deps = [
    cc.find_library('rt'),
    cc.find_library('m'),
    dependency('libipmb'),
    dependency('libipmi'),
    dependency('libkv'),
    dependency('threads'),
    dependency('libsnr-tolerance'),
    ]

# Import the plat-specific subdirectory for platform
# headers/sources/dependencies.
subdir('plat')

if build_crashdump_amd
    add_project_arguments('-DCRASHDUMP_AMD', language : 'c')
    pal_sources += 'crashdump-amd/pal_crashdump_amd.c'
    pal_headers += 'crashdump-amd/pal_crashdump_amd.h'
endif

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
