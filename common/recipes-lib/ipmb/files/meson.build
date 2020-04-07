project('libipmb', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers('ipmb.h', subdir: 'openbmc')

thread_lib = dependency('threads')
ipc_lib = dependency('libipc')

ipmb_lib = shared_library('ipmb',
    'ipmb.c',
    dependencies: [thread_lib, ipc_lib],
    version: meson.project_version(),
    install: true)

pkg = import('pkgconfig')
pkg.generate(libraries: [ipmb_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'IPMB client library')
