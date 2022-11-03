project('dimm-util',
    'cpp',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'cpp_std=c++1z',
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

sources = [
    'dimm-capacity.cpp',
    'dimm-pmic.cpp',
    'dimm-util.cpp',
    'dimm-vendor.cpp',
]

deps = [
    dependency('jansson'),
    dependency('libkv'),
    dependency('obmc-pal'),
]

subdir('plat')

exe = executable('dimm-util',
    sources,
    dependencies: deps,
    install: true,
)
