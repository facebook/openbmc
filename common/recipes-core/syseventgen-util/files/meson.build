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

syseventgen_headers = files (
    'syseventgen-util.h'
)

syseventgen_sources = files (
    'syseventgen-util.cpp'
)

# Make the executable
executable('syseventgen-util',
    syseventgen_sources,
    dependencies: syseventgen_deps,
    install:true,
    install_dir: 'local/bin'
)
