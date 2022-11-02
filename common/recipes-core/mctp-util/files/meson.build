project('mctp-util',
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
    'decode.cpp',
    'mctp-util.cpp',
]

deps = [
    dependency('libobmc-mctp'),
    dependency('obmc-pal'),
]

exe = executable('mctp-util',
    sources,
    dependencies: deps,
    install: true,
)
