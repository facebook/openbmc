project('libnetlakemtp_fruid', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers('netlakemtp_fruid.h', subdir: 'facebook')

cc = meson.get_compiler('c')

netlakemtp_fruid_deps = [
    cc.find_library('netlakemtp_common'),
]

# netlakemtp fruid library.
netlakemtp_fruid_lib = shared_library('netlakemtp_fruid', 'netlakemtp_fruid.c',
    dependencies: netlakemtp_fruid_deps,
    version: meson.project_version(),
    install: true)

# pkgconfig for netlakemtp fruid library.
pkg = import('pkgconfig')
pkg.generate(libraries: [netlakemtp_fruid_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'netlakemtp fruid Library')
