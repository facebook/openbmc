# Copyright 2023-present Meta Platform. All Rights Reserved.
project('weutil', 'cpp',
    version: '0.1',
    license: 'GPL2',
    # Meson 0.40 only supports c++1z as an alias for c++17.
    default_options: [
        'cpp_std=c++1z',
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

cc = meson.get_compiler('cpp')

dep_libs = [
    cc.find_library('wedge-eeprom'),
    ]

# weutil executable.
weutil_exe = executable('weutil', 'Weutil.cpp',
    dependencies: dep_libs,
    install: true)

