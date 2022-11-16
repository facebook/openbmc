project('fan-util',
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
    'fan-util.cpp',
]

deps = [
    dependency('obmc-pal'),
    dependency('threads'),
]

exe = executable('fan-util',
    sources,
    dependencies: deps,
    install: true,
)
