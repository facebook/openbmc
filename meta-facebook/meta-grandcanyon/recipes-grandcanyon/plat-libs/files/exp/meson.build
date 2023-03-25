project('libexp', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers('exp.h', subdir: 'facebook')

cc = meson.get_compiler('c')

dep_libs = [
    dependency('libipmb'),
    dependency('libfbgc_common'),
    ]

# Expander library.
exp_lib = shared_library('exp', 'exp.c',
    dependencies: dep_libs,
    version: meson.project_version(),
    install: true)

# pkgconfig for Expander library.
pkg = import('pkgconfig')
pkg.generate(libraries: [exp_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Expander Library')
