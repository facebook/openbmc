project('syseventgen-util', 'cpp',
    version: '0.1',
    license: 'GPLv2',
    default_options: ['werror=true', 'cpp_std=c++17'],
    meson_version: '>=0.40')

syseventgen_headers = [
    'syseventgen-util.h'
]

syseventgen_sources = [
    'syseventgen-util.cpp'
]

install_headers(
    syseventgen_headers,
    subdir: 'openbmc')

# Make the executable
executable('syseventgen-util', 'syseventgen-util.cpp', install:true, install_dir:'/local/bin')
