project('libnetlakenext_fruid', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers('netlakenext_fruid.h', subdir: 'facebook')

cc = meson.get_compiler('c')

netlakenext_fruid_deps = [
    cc.find_library('netlakenext_common'),
]

# netlakenext fruid library.
netlakenext_fruid_lib = shared_library('netlakenext_fruid', 'netlakenext_fruid.c',
    dependencies: netlakenext_fruid_deps,
    version: meson.project_version(),
    install: true)

# pkgconfig for netlakenext fruid library.
pkg = import('pkgconfig')
pkg.generate(libraries: [netlakenext_fruid_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'netlakenext fruid Library')
