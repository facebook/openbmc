project('syseventgen-util', 'c', 'cpp',
    version: '0.1',
    license: 'GPLv2',
    default_options: ['werror=true', 'cpp_std=c++1z'],
    meson_version: '>=0.40')

cc = meson.get_compiler('c')
syseventgen_deps = [
    cc.find_library('pal'),
    cc.find_library('sdr')
]

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
executable('syseventgen-util',
    syseventgen_sources,
    dependencies: syseventgen_deps,
    install:true,
    install_dir: 'local/bin'
)
