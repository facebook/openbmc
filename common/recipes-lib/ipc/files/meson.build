project('libipc', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers('ipc.h', subdir: 'openbmc')

thread_lib = dependency('threads')

ipc_lib = shared_library('ipc',
    'ipc.c',
    dependencies: thread_lib,
    version: meson.project_version(),
    install: true)

pkg = import('pkgconfig')
pkg.generate(libraries: [ipc_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'ipc abstraction library')
