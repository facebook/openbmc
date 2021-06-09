project('syseventgen-util', 'cpp',
    version: '0.1',
    license: 'GPLv2',
    default_options: ['werror=true', 'cpp_std=c++17'],
    meson_version: '>=0.40')

ccpp = meson.get_compiler('cpp')

# syseventgen_deps = [
#
# ]

syseventgen_headers = [
    'syseventgen-util.h'
]

syseventgen_sources = [
    'syseventgen-util.cpp'
]

# subdir('plat')

install_headers(
    syseventgen_headers,
    subdir: 'openbmc')

# syseventgen_lib = shared_library('syseventgen-util',
#    syseventgen_sources,
#    dependencies: syseventgen_deps,
#    version: meson.project_version(),
#    install: true)

# Make the executable
executable('syseventgen-util', 'syseventgen-util.cpp', install:true)
