project('emmcd', 'c', 'cpp',
    version: '0.1',
    license: 'GPL2',
    # Meson 0.40 only supports c++1z as an alias for c++17.
    default_options: [
        'cpp_std=c++1z',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

cc = meson.get_compiler('c')
deps = [
    dependency('liblog'),
    dependency('libmisc-utils'),
    dependency('threads'),
]

srcs = files(
    'emmcd.c',
)

emmcd_exe = executable('emmcd', srcs,
    dependencies: deps,
    install: true,
)
