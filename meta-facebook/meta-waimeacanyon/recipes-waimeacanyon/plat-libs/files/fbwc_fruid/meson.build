project('libfbwc_fruid', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers('fbwc_fruid.h', subdir: 'facebook')

cc = meson.get_compiler('c')

fbwc_fruid_deps = [
    dependency('libfbwc_common'),
    dependency('libfruid'),
    dependency('libbic'),
    dependency('libexp'),
]

# FBWC fruid library.
fbwc_fruid_lib = shared_library('fbwc_fruid', 'fbwc_fruid.c',
    dependencies: fbwc_fruid_deps,
    version: meson.project_version(),
    install: true)

# pkgconfig for FBWC fruid library.
pkg = import('pkgconfig')
pkg.generate(libraries: [fbwc_fruid_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'FBWC fruid Library')
