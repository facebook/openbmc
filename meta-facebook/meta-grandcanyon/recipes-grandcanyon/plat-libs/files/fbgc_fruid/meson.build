project('libfbgc_fruid', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers('fbgc_fruid.h', subdir: 'facebook')

cc = meson.get_compiler('c')

fbgc_fruid_deps = [
    dependency('libfbgc_common'),
    dependency('libbic'),
    dependency('libfruid'),
]

# FBGC fruid library.
fbgc_fruid_lib = shared_library('fbgc_fruid', 'fbgc_fruid.c',
    dependencies: fbgc_fruid_deps,
    version: meson.project_version(),
    install: true)

# pkgconfig for FBGC fruid library.
pkg = import('pkgconfig')
pkg.generate(libraries: [fbgc_fruid_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'FBGC fruid Library')
